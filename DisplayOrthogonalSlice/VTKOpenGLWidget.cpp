#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkNrrdReader.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_leftRenderer(vtkSmartPointer<vtkRenderer>::New())
    , m_middleRenderer(vtkSmartPointer<vtkRenderer>::New())
    , m_rightRenderer(vtkSmartPointer<vtkRenderer>::New())
{
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget()
{

}

void VTKOpenGLWidget::initialize()
{
    m_leftRenderer->SetViewport(0, 0, 0.33, 1);
    m_leftRenderer->SetBackground(1.0, 0, 0);

    m_middleRenderer->SetViewport(0.33, 0, 0.66, 1);
    m_middleRenderer->SetBackground(0, 1.0, 0);

    m_rightRenderer->SetViewport(0.66, 0, 1.0, 1);
    m_rightRenderer->SetBackground(0, 0.0, 1.0);

    m_renderWindow->AddRenderer(m_leftRenderer);
    m_renderWindow->AddRenderer(m_middleRenderer);
    m_renderWindow->AddRenderer(m_rightRenderer);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData()
{
    // std::string fileName = ""
    // vtkNew<vtkNrrdReader> reader;
    // reader->SetFileName();
}
