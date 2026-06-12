#include "VTKOpenGLWidget.h"

#include <QMouseEvent>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageEllipsoidSource.h>
#include <vtkImageMapper3D.h>
#include <vtkImageProperty.h>
#include <vtkImageReslice.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLineSource.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include <algorithm>

namespace {

// Reslice axes direction cosines (view x, view y, slice normal) per orientation.
// View coordinates of a world point p:
//   Sagittal:   (p.y, p.z), slicing along world X
//   Coronal:    (p.x, p.z), slicing along world Y
//   Transverse: (p.x, p.y), slicing along world Z
const double kResliceAxes[3][9] = {
    {0, 1, 0, 0, 0, 1, 1, 0, 0},  // Sagittal
    {1, 0, 0, 0, 0, 1, 0, -1, 0}, // Coronal
    {1, 0, 0, 0, 1, 0, 0, 0, 1},  // Transverse
};

// Crosshair lines lie slightly above the slice so they are always visible.
const double kCrosshairZ = 1.0;

// Each plane has a fixed color; a view draws the lines of the two other planes.
const double kPlaneColors[3][3] = {
    {1.0, 1.0, 0.0}, // Sagittal: yellow
    {1.0, 0.0, 0.0}, // Coronal: red
    {0.0, 1.0, 0.0}, // Transverse: green
};

vtkSmartPointer<vtkActor> createLineActor(vtkLineSource *line, const double color[3]) {
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(line->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(color[0], color[1], color[2]);
    actor->GetProperty()->SetLineWidth(1.5);
    return actor;
}

} // namespace

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_activeViewIndex(-1) {
    for (int i = 0; i < 3; ++i) {
        m_views[i].renderer = vtkSmartPointer<vtkRenderer>::New();
        m_views[i].orientation = i;
    }
    initialize();
    createTestData();
}

void VTKOpenGLWidget::initialize() {
    // 2x2 layout: transverse top-left, sagittal top-right, coronal bottom-left
    m_views[Transverse].renderer->SetViewport(0.0, 0.5, 0.5, 1.0);
    m_views[Sagittal].renderer->SetViewport(0.5, 0.5, 1.0, 1.0);
    m_views[Coronal].renderer->SetViewport(0.0, 0.0, 0.5, 0.5);

    m_views[Transverse].renderer->SetBackground(0.0, 0.0, 0.0);
    m_views[Sagittal].renderer->SetBackground(0.05, 0.05, 0.05);
    m_views[Coronal].renderer->SetBackground(0.1, 0.1, 0.1);

    for (int i = 0; i < 3; ++i) {
        m_renderWindow->AddRenderer(m_views[i].renderer);
    }
    SetRenderWindow(m_renderWindow);

    vtkNew<vtkInteractorStyleImage> style;
    GetInteractor()->SetInteractorStyle(style);
}

void VTKOpenGLWidget::createTestData() {
    // Placeholder volume until a real medical image is loaded.
    // Different radii per axis so the three views look different.
    vtkNew<vtkImageEllipsoidSource> ellipsoid;
    ellipsoid->SetWholeExtent(0, 99, 0, 99, 0, 99);
    ellipsoid->SetCenter(50.0, 50.0, 50.0);
    ellipsoid->SetRadius(40.0, 25.0, 15.0);
    ellipsoid->SetInValue(255.0);
    ellipsoid->SetOutValue(0.0);
    ellipsoid->Update();

    m_image = ellipsoid->GetOutput();
    m_image->GetCenter(m_crosshair);

    for (int i = 0; i < 3; ++i) {
        setupSliceView(m_views[i], m_image);
    }
}

void VTKOpenGLWidget::setupSliceView(SliceView &view, vtkImageData *image) {
    view.reslice = vtkSmartPointer<vtkImageReslice>::New();
    view.reslice->SetInputData(image);
    view.reslice->SetOutputDimensionality(2);
    const double *axes = kResliceAxes[view.orientation];
    view.reslice->SetResliceAxesDirectionCosines(axes[0], axes[1], axes[2],
                                                 axes[3], axes[4], axes[5],
                                                 axes[6], axes[7], axes[8]);
    view.reslice->SetInterpolationModeToLinear();
    updateResliceOrigin(view);

    // The in-plane components of the reslice origin never change, so these
    // bounds stay valid as the crosshair moves.
    view.reslice->Update();
    view.reslice->GetOutput()->GetBounds(view.bounds);

    vtkNew<vtkImageActor> actor;
    actor->GetMapper()->SetInputConnection(view.reslice->GetOutputPort());
    actor->GetProperty()->SetColorWindow(255.0);
    actor->GetProperty()->SetColorLevel(127.5);
    view.renderer->AddViewProp(actor);

    // Lines are colored by the plane they represent in this view.
    int horizontalPlane = 0;
    int verticalPlane = 0;
    switch (view.orientation) {
    case Sagittal:
        horizontalPlane = Transverse;
        verticalPlane = Coronal;
        break;
    case Coronal:
        horizontalPlane = Transverse;
        verticalPlane = Sagittal;
        break;
    case Transverse:
        horizontalPlane = Coronal;
        verticalPlane = Sagittal;
        break;
    }

    view.horizontalLine = vtkSmartPointer<vtkLineSource>::New();
    view.verticalLine = vtkSmartPointer<vtkLineSource>::New();
    updateCrosshairLines(view);
    view.renderer->AddActor(createLineActor(view.horizontalLine, kPlaneColors[horizontalPlane]));
    view.renderer->AddActor(createLineActor(view.verticalLine, kPlaneColors[verticalPlane]));

    const double centerX = 0.5 * (view.bounds[0] + view.bounds[1]);
    const double centerY = 0.5 * (view.bounds[2] + view.bounds[3]);
    vtkCamera *camera = view.renderer->GetActiveCamera();
    camera->ParallelProjectionOn();
    camera->SetFocalPoint(centerX, centerY, 0.0);
    camera->SetPosition(centerX, centerY, 500.0);
    camera->SetViewUp(0.0, 1.0, 0.0);
    view.renderer->ResetCamera();
}

void VTKOpenGLWidget::updateResliceOrigin(SliceView &view) {
    switch (view.orientation) {
    case Sagittal:
        view.reslice->SetResliceAxesOrigin(m_crosshair[0], 0.0, 0.0);
        break;
    case Coronal:
        view.reslice->SetResliceAxesOrigin(0.0, m_crosshair[1], 0.0);
        break;
    case Transverse:
        view.reslice->SetResliceAxesOrigin(0.0, 0.0, m_crosshair[2]);
        break;
    }
}

void VTKOpenGLWidget::crosshairToViewCoords(const SliceView &view, double &x,
                                            double &y) const {
    switch (view.orientation) {
    case Sagittal:
        x = m_crosshair[1];
        y = m_crosshair[2];
        break;
    case Coronal:
        x = m_crosshair[0];
        y = m_crosshair[2];
        break;
    case Transverse:
        x = m_crosshair[0];
        y = m_crosshair[1];
        break;
    }
}

void VTKOpenGLWidget::updateCrosshairLines(SliceView &view) {
    double x = 0.0;
    double y = 0.0;
    crosshairToViewCoords(view, x, y);

    view.horizontalLine->SetPoint1(view.bounds[0], y, kCrosshairZ);
    view.horizontalLine->SetPoint2(view.bounds[1], y, kCrosshairZ);
    view.verticalLine->SetPoint1(x, view.bounds[2], kCrosshairZ);
    view.verticalLine->SetPoint2(x, view.bounds[3], kCrosshairZ);
}

void VTKOpenGLWidget::setCrosshairPosition(double x, double y, double z) {
    double bounds[6];
    m_image->GetBounds(bounds);
    m_crosshair[0] = std::min(std::max(x, bounds[0]), bounds[1]);
    m_crosshair[1] = std::min(std::max(y, bounds[2]), bounds[3]);
    m_crosshair[2] = std::min(std::max(z, bounds[4]), bounds[5]);

    for (int i = 0; i < 3; ++i) {
        updateResliceOrigin(m_views[i]);
        updateCrosshairLines(m_views[i]);
    }
    m_renderWindow->Render();
}

bool VTKOpenGLWidget::event(QEvent *event) {
    // Intercept before QVTKOpenGLNativeWidget forwards the event to the VTK
    // interactor, so the left button is reserved for the crosshair.
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        if (handleCrosshairMouseEvent(static_cast<QMouseEvent *>(event))) {
            return true;
        }
        break;
    default:
        break;
    }
    return QVTKOpenGLNativeWidget::event(event);
}

bool VTKOpenGLWidget::handleCrosshairMouseEvent(QMouseEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        if (event->button() != Qt::LeftButton) {
            return false;
        }
        int x = 0;
        int y = 0;
        toVtkDisplayCoords(event, x, y);

        m_activeViewIndex = -1;
        vtkRenderer *poked = GetInteractor()->FindPokedRenderer(x, y);
        for (int i = 0; i < 3; ++i) {
            if (m_views[i].renderer.GetPointer() == poked) {
                m_activeViewIndex = i;
            }
        }
        if (m_activeViewIndex < 0) {
            return false;
        }
        moveCrosshairToDisplayPoint(x, y);
        return true;
    }

    if (event->type() == QEvent::MouseMove) {
        if (m_activeViewIndex < 0 || !(event->buttons() & Qt::LeftButton)) {
            return false;
        }
        int x = 0;
        int y = 0;
        toVtkDisplayCoords(event, x, y);
        moveCrosshairToDisplayPoint(x, y);
        return true;
    }

    // MouseButtonRelease
    if (event->button() == Qt::LeftButton && m_activeViewIndex >= 0) {
        m_activeViewIndex = -1;
        return true;
    }
    return false;
}

void VTKOpenGLWidget::toVtkDisplayCoords(QMouseEvent *event, int &x, int &y) const {
    // Qt has its origin top-left; VTK display coordinates start bottom-left
    // and may be scaled on high-DPI screens.
    const int *size = m_renderWindow->GetSize();
    const double scaleX = static_cast<double>(size[0]) / std::max(1, width());
    const double scaleY = static_cast<double>(size[1]) / std::max(1, height());
    x = static_cast<int>(event->x() * scaleX);
    y = size[1] - 1 - static_cast<int>(event->y() * scaleY);
}

void VTKOpenGLWidget::moveCrosshairToDisplayPoint(int x, int y) {
    SliceView &view = m_views[m_activeViewIndex];
    vtkRenderer *renderer = view.renderer;

    // Project the display point onto the slice plane (z = 0 in view coords).
    double focalPoint[3];
    renderer->GetActiveCamera()->GetFocalPoint(focalPoint);
    renderer->SetWorldPoint(focalPoint[0], focalPoint[1], focalPoint[2], 1.0);
    renderer->WorldToDisplay();
    const double displayZ = renderer->GetDisplayPoint()[2];

    renderer->SetDisplayPoint(x, y, displayZ);
    renderer->DisplayToWorld();
    double world[4];
    renderer->GetWorldPoint(world);

    double pos[3] = {m_crosshair[0], m_crosshair[1], m_crosshair[2]};
    switch (view.orientation) {
    case Sagittal:
        pos[1] = world[0];
        pos[2] = world[1];
        break;
    case Coronal:
        pos[0] = world[0];
        pos[2] = world[1];
        break;
    case Transverse:
        pos[0] = world[0];
        pos[1] = world[1];
        break;
    }
    setCrosshairPosition(pos[0], pos[1], pos[2]);
}
