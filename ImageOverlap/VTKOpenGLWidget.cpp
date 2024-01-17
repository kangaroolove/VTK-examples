#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkNrrdReader.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkImageStack.h>
#include <vtkPlane.h>

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
    std::vector<vtkImageStack*> stacks = { vtkImageStack::New(), vtkImageStack::New(), vtkImageStack::New() };
    for (int i = 0; i < 3; i++)
    {
        m_renderer->AddActor(stacks[i]);
    }

    vtkNew<vtkNrrdReader> nrrdReader;
    nrrdReader->SetFileName("D:/MRI.nrrd");

    for (int i = 0; i < 3; i++)
    {
        double normal[3] = { 0 };
        normal[i] = 1;
        vtkNew<vtkPlane> plane;
        plane->SetOrigin(50, 50, 50);
        plane->SetNormal(normal);

        vtkNew<vtkImageResliceMapper> mapper;
        mapper->SetInputConnection(nrrdReader->GetOutputPort());
        mapper->SetSlicePlane(plane);

        vtkNew<vtkImageSlice> slice;
        slice->SetMapper(mapper);

        stacks[i]->AddImage(slice);
    }
}
