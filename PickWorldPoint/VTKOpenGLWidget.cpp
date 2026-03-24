#include "VTKOpenGLWidget.h"

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkConeSource.h>
#include <vtkCoordinate.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkInteractorStyleImage.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkWorldPointPicker.h>

#include <QDebug>
#include <QMouseEvent>
#include <array>
class MouseInteractorStyle : public vtkInteractorStyleImage {
public:
    static MouseInteractorStyle *New();
    vtkTypeMacro(MouseInteractorStyle, vtkInteractorStyleImage);

    void setRenderer(vtkRenderer *renderer) { m_renderer = renderer; }

    // void OnMouseMove() override {
    //     std::array<int, 2> eventPosition;
    //     this->GetInteractor()->GetEventPosition(eventPosition.data());

    //     qDebug() << "eventPosition " << eventPosition[0] << ", "
    //              << eventPosition[1];

    //     auto camera = m_renderer->GetActiveCamera();
    //     std::array<double, 3> focalPoint;
    //     camera->GetFocalPoint(focalPoint.data());

    //     // std::cout << "camera focalPoint " << focalPoint[0] << ","
    //     //           << focalPoint[1] << "," << focalPoint[2] << endl;

    //     std::array<double, 3> displayPoint;
    //     ComputeWorldToDisplay(focalPoint[0], focalPoint[1], focalPoint[2],
    //                           displayPoint.data());

    //     auto focalDepth = displayPoint[2];

    //     std::array<double, 4> worldPoint;
    //     ComputeDisplayToWorld(eventPosition[0], eventPosition[1], focalDepth,
    //                           worldPoint.data());

    //     // std::cout << "worldPoint " << worldPoint[0] << "," << worldPoint[1]
    //     //           << "," << worldPoint[2] << endl;

    //     vtkNew<vtkCoordinate> coordinate;
    //     coordinate->SetCoordinateSystemToDisplay();
    //     coordinate->SetValue(eventPosition[0], eventPosition[1], 0);
    //     double *worldPosition = coordinate->GetComputedWorldValue(m_renderer);
    //     // std::cout << "coordinate " << worldPosition[0] << ","
    //     //           << worldPosition[1] << "," << worldPosition[2] << endl;
    //     vtkInteractorStyleImage::OnMouseMove();
    // }
    void OnLeftButtonDown() override {
        std::array<int, 2> eventPosition;
        this->GetInteractor()->GetEventPosition(eventPosition.data());

        qDebug() << "eventPosition " << eventPosition[0] << ", "
                 << eventPosition[1];

        // world picker
        // This result is totally wrong
        // vtkWorldPointPicker reads the z-buffer at that pixel. If no geometry
        // is rendered there,the z-buffer returns a default depth (near/far
        // plane), leading to a nonsensical X,Y result. The result does not
        // correspond to the actual clicked screen position — unreliable without
        // geometry under the cursor
        vtkNew<vtkWorldPointPicker> picker;
        picker->Pick(eventPosition[0], eventPosition[1], 0.0, m_renderer);

        double worldPickerPoint[3];
        picker->GetPickPosition(worldPickerPoint);

        std::cout << "World picker point: " << worldPickerPoint[0] << ", "
                  << worldPickerPoint[1] << ", " << worldPickerPoint[2]
                  << std::endl;

        // Interator observer
        // This is the correct way to get the world point
        auto camera = m_renderer->GetActiveCamera();
        std::array<double, 3> focalPoint;
        camera->GetFocalPoint(focalPoint.data());

        std::cout << "camera focalPoint " << focalPoint[0] << ","
                  << focalPoint[1] << "," << focalPoint[2] << endl;

        std::array<double, 3> displayPoint;
        ComputeWorldToDisplay(focalPoint[0], focalPoint[1], focalPoint[2],
                              displayPoint.data());

        auto focalDepth = displayPoint[2];

        std::array<double, 4> worldPoint;
        ComputeDisplayToWorld(eventPosition[0], eventPosition[1], focalDepth,
                              worldPoint.data());

        std::cout << "worldPoint " << worldPoint[0] << "," << worldPoint[1]
                  << "," << worldPoint[2] << endl;

        // VTK coordinate
        // This result is wrong since the z buffer is 0
        vtkNew<vtkCoordinate> coordinate;
        coordinate->SetCoordinateSystemToDisplay();
        coordinate->SetValue(eventPosition[0], eventPosition[1], 0);
        double *coordinateWorldPos = coordinate->GetComputedWorldValue(m_renderer);
        std::cout << "coordinate " << coordinateWorldPos[0] << ","
                  << coordinateWorldPos[1] << "," << coordinateWorldPos[2] << endl;

        vtkInteractorStyleImage::OnLeftButtonDown();
    }

private:
    vtkRenderer *m_renderer;
};
vtkStandardNewMacro(MouseInteractorStyle);

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_renderer(vtkSmartPointer<vtkRenderer>::New()) {
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

// void VTKOpenGLWidget::mouseMoveEvent(QMouseEvent *event) {
//     qDebug() << event->pos();
//     QVTKOpenGLNativeWidget::mouseMoveEvent(event);
// }

void VTKOpenGLWidget::initialize() {
    vtkNew<MouseInteractorStyle> it;
    it->setRenderer(m_renderer);

    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);

    m_renderWindow->GetInteractor()->SetInteractorStyle(it);
    resize(800, 600);
}

void VTKOpenGLWidget::createTestData() {
    vtkNew<vtkConeSource> cone;

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(cone->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    m_renderer->AddActor(actor);
}
