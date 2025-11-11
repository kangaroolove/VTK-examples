#include "VTKOpenGLWidget.h"

#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkDataSetMapper.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_renderer(vtkSmartPointer<vtkRenderer>::New()),
      m_markerWidget(vtkSmartPointer<vtkOrientationMarkerWidget>::New()) {
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData() {
    vtkNew<vtkXMLPolyDataReader> reader;
    reader->SetFileName("D:/work/VTK-example/OrientationMarker/resource/Human.vtp");
    reader->Update();
    reader->GetOutput()->GetPointData()->SetActiveScalars("Color");

    vtkNew<vtkDataSetMapper> markerMapper;
    markerMapper->SetInputConnection(reader->GetOutputPort());
    markerMapper->SetColorModeToDirectScalars();

    vtkNew<vtkActor> markerActor;
    markerActor->SetMapper(markerMapper);

    m_markerWidget->SetOrientationMarker(markerActor);
    m_markerWidget->SetInteractor(m_renderWindow->GetInteractor());
    m_markerWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
    m_markerWidget->SetEnabled(1);
    m_markerWidget->InteractiveOff();

    vtkNew<vtkConeSource> cone;

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(cone->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    m_renderer->AddActor(actor);
    m_renderer->ResetCamera();
}
