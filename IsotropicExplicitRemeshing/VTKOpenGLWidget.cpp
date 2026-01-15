#include "VTKOpenGLWidget.h"
#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkIsotropicDiscreteRemeshing.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_leftRenderer(vtkSmartPointer<vtkRenderer>::New()),
      m_rightRenderer(vtkSmartPointer<vtkRenderer>::New()) {
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
    m_leftRenderer->SetBackground(1.0, 0.0, 0.0);
    m_leftRenderer->SetViewport(0.0, 0.0, 0.5, 1.0);
    m_renderWindow->AddRenderer(m_leftRenderer);

    m_rightRenderer->SetBackground(0.0, 1.0, 0.0);
    m_rightRenderer->SetViewport(0.5, 0, 1.0, 1.0);
    m_renderWindow->AddRenderer(m_rightRenderer);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData() {
    vtkNew<vtkConeSource> cone;

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(cone->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    m_leftRenderer->AddActor(actor);
}
