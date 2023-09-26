#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkSphere.h>
#include <vtkVectorText.h>
#include <vtkPolyDataMapper.h>
#include <vtkFollower.h>
#include <vtkSphereSource.h>
#include <vtkCylinderSource.h>
#include <vtkTransformFilter.h>
#include <vtkTransform.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_renderer(vtkSmartPointer<vtkRenderer>::New())
{
    initialize();
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

void VTKOpenGLWidget::createTestData()
{
    // vtkNew<vtkConeSource> cone;

    // vtkNew<vtkPolyDataMapper> mapper;
    // mapper->SetInputConnection(cone->GetOutputPort());

    // vtkNew<vtkActor> actor;
    // actor->SetMapper(mapper);

    // m_renderer->AddActor(actor);


    vtkNew<vtkCylinderSource> cylinder;
    cylinder->SetRadius(5);
    cylinder->SetHeight(20);
    cylinder->SetResolution(100);

    vtkNew<vtkTransform> cylinderTransform;
    cylinderTransform->RotateX(90);

    vtkNew<vtkTransformFilter> cylinderTransformFilter;
    cylinderTransformFilter->SetInputConnection(cylinder->GetOutputPort());
    cylinderTransformFilter->SetTransform(cylinderTransform);

    vtkNew<vtkPolyDataMapper> cylinderMapper;
    cylinderMapper->SetInputConnection(cylinderTransformFilter->GetOutputPort());

    vtkNew<vtkActor> cylinderActor;
    cylinderActor->SetMapper(cylinderMapper);

    m_renderer->AddActor(cylinderActor);




    vtkNew<vtkVectorText> text;
    text->SetText("ABC");

    vtkNew<vtkTransform> textTransform;
    textTransform->Translate(-2, 10, 0);

    vtkNew<vtkTransformFilter> textTransformFilter;
    textTransformFilter->SetInputConnection(text->GetOutputPort());
    textTransformFilter->SetTransform(textTransform);

    vtkNew<vtkPolyDataMapper> textMapper;
    textMapper->SetInputConnection(textTransformFilter->GetOutputPort());

    vtkNew<vtkFollower> follower;
    follower->SetMapper(textMapper);
    follower->SetCamera(m_renderer->GetActiveCamera());
    m_renderer->AddActor(follower);

    m_renderer->ResetCamera();
}
