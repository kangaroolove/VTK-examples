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
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageToVTKImageFilter.h>
#include <itkNrrdImageIO.h>

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

    vtkNew<vtkMatrix4x4> identity;
    setImage(ellipsoid->GetOutput(), identity);
}

bool VTKOpenGLWidget::loadImage(const QString &fileName, QString &errorMessage) {
    typedef itk::Image<float, 3> ImageType;

    const std::string path = fileName.toStdString();

    itk::NrrdImageIO::Pointer io = itk::NrrdImageIO::New();
    if (!io->CanReadFile(path.c_str())) {
        errorMessage = tr("Only NRRD images (*.nrrd, *.nhdr) are supported:\n%1")
                           .arg(fileName);
        return false;
    }

    // ITK converts whatever space the NRRD header declares (RAS, LAS, ...)
    // to LPS, so the origin and direction below are always in LPS.
    itk::ImageFileReader<ImageType>::Pointer reader =
        itk::ImageFileReader<ImageType>::New();
    reader->SetImageIO(io);
    reader->SetFileName(path);
    try {
        reader->Update();
    } catch (itk::ExceptionObject &e) {
        errorMessage = tr("Failed to read image:\n%1\n%2")
                           .arg(fileName)
                           .arg(QString::fromLatin1(e.GetDescription()));
        return false;
    }

    ImageType::Pointer itkImage = reader->GetOutput();

    // vtkImageData cannot store direction cosines, so keep the voxel grid in
    // axis-aligned image coordinates (spacing only) and carry rotation and
    // origin in a separate image-to-world matrix.
    const ImageType::DirectionType direction = itkImage->GetDirection();
    const ImageType::PointType origin = itkImage->GetOrigin();
    vtkNew<vtkMatrix4x4> imageToWorld;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            imageToWorld->SetElement(row, col, direction(row, col));
        }
        imageToWorld->SetElement(row, 3, origin[row]);
    }

    typedef itk::ImageToVTKImageFilter<ImageType> ConnectorType;
    ConnectorType::Pointer connector = ConnectorType::New();
    connector->SetInput(itkImage);
    connector->Update();

    vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
    image->DeepCopy(connector->GetOutput());
    image->SetOrigin(0.0, 0.0, 0.0); // origin lives in imageToWorld

    setImage(image, imageToWorld);
    m_renderWindow->Render();
    return true;
}

void VTKOpenGLWidget::setImage(vtkImageData *image, vtkMatrix4x4 *imageToWorld) {
    m_image = image;
    m_imageToWorld = imageToWorld;
    updateWorldBounds();

    m_crosshair[0] = 0.5 * (m_worldBounds[0] + m_worldBounds[1]);
    m_crosshair[1] = 0.5 * (m_worldBounds[2] + m_worldBounds[3]);
    m_crosshair[2] = 0.5 * (m_worldBounds[4] + m_worldBounds[5]);

    for (int i = 0; i < 3; ++i) {
        setupSliceView(m_views[i], m_image);
    }
}

void VTKOpenGLWidget::updateWorldBounds() {
    double imageBounds[6];
    m_image->GetBounds(imageBounds);

    // World bounds are the axis-aligned box around the eight transformed
    // corners of the image-space bounding box.
    for (int corner = 0; corner < 8; ++corner) {
        const double point[4] = {imageBounds[corner & 1],
                                 imageBounds[2 + ((corner >> 1) & 1)],
                                 imageBounds[4 + ((corner >> 2) & 1)], 1.0};
        double world[4];
        m_imageToWorld->MultiplyPoint(point, world);
        for (int axis = 0; axis < 3; ++axis) {
            if (corner == 0) {
                m_worldBounds[2 * axis] = world[axis];
                m_worldBounds[2 * axis + 1] = world[axis];
            } else {
                m_worldBounds[2 * axis] = std::min(m_worldBounds[2 * axis], world[axis]);
                m_worldBounds[2 * axis + 1] = std::max(m_worldBounds[2 * axis + 1], world[axis]);
            }
        }
    }
}

void VTKOpenGLWidget::setupSliceView(SliceView &view, vtkImageData *image) {
    // A new image replaces the previous slice and crosshair actors.
    view.renderer->RemoveAllViewProps();

    view.reslice = vtkSmartPointer<vtkImageReslice>::New();
    view.reslice->SetInputData(image);
    view.reslice->SetOutputDimensionality(2);
    const double *axes = kResliceAxes[view.orientation];
    view.reslice->SetResliceAxesDirectionCosines(axes[0], axes[1], axes[2],
                                                 axes[3], axes[4], axes[5],
                                                 axes[6], axes[7], axes[8]);

    // The reslice axes are world-aligned (LPS) planes; the reslice transform
    // maps those world coordinates into the image's own grid, applying the
    // volume's rotation and origin.
    vtkNew<vtkMatrix4x4> worldToImage;
    vtkMatrix4x4::Invert(m_imageToWorld, worldToImage);
    vtkNew<vtkTransform> resliceTransform;
    resliceTransform->SetMatrix(worldToImage);
    view.reslice->SetResliceTransform(resliceTransform);
    view.reslice->AutoCropOutputOn();
    view.reslice->SetInterpolationModeToLinear();

    double range[2];
    image->GetScalarRange(range);
    if (range[1] <= range[0]) {
        range[1] = range[0] + 1.0;
    }
    view.reslice->SetBackgroundLevel(range[0]);
    updateResliceOrigin(view);

    // The in-plane components of the reslice origin never change, so these
    // bounds stay valid as the crosshair moves.
    view.reslice->Update();
    view.reslice->GetOutput()->GetBounds(view.bounds);

    vtkNew<vtkImageActor> actor;
    actor->GetMapper()->SetInputConnection(view.reslice->GetOutputPort());
    actor->GetProperty()->SetColorWindow(range[1] - range[0]);
    actor->GetProperty()->SetColorLevel(0.5 * (range[0] + range[1]));
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
    m_crosshair[0] = std::min(std::max(x, m_worldBounds[0]), m_worldBounds[1]);
    m_crosshair[1] = std::min(std::max(y, m_worldBounds[2]), m_worldBounds[3]);
    m_crosshair[2] = std::min(std::max(z, m_worldBounds[4]), m_worldBounds[5]);

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
