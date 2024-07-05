#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkImageActor.h>
#include <vtkNrrdReader.h>
#include <vtkInteractorStyleImage.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_renderer(vtkSmartPointer<vtkRenderer>::New())
{
    initialize();
    initInteraction();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget()
{

}

void VTKOpenGLWidget::initialize()
{
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::initInteraction()
{
    vtkNew<vtkInteractorStyleImage> style;
    m_renderWindow->GetInteractor()->SetInteractorStyle(style);
}

void VTKOpenGLWidget::createTestData()
{
    vtkNew<vtkNrrdReader> reader;
    reader->SetFileName("D:/CBJ_T2.nrrd");
    reader->Update();

    vtkNew<vtkImageActor> actor;
    actor->SetInputData(reader->GetOutput());

    m_renderer->AddViewProp(actor);
}
