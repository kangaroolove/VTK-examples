#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkNrrdReader.h>
#include <vtkImageReslice.h>
#include <vtkImageSlice.h>
#include <vtkImageSliceMapper.h>
#include <vtkOutlineFilter.h>
#include <vtkTransform.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageProperty.h>
#include <vtkPlane.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_leftRenderer(vtkSmartPointer<vtkRenderer>::New())
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
    m_renderWindow->AddRenderer(m_leftRenderer);
    m_leftRenderer->SetBackground(0.5, 0.5, 0.5);
    m_leftRenderer->SetViewport(0, 0, 0.5, 1);
    m_renderWindow->AddRenderer(m_rightRenderer);
    m_rightRenderer->SetBackground(0, 1.0, 0);
    m_rightRenderer->SetViewport(0.5, 0, 1, 1);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData()
{
    vtkNew<vtkNrrdReader> reader;
    reader->SetFileName("D:/MRI.nrrd");

    vtkNew<vtkTransform> transform;
    //transform->RotateY(-90);

    vtkNew<vtkImageReslice> imageReslice;
    imageReslice->SetInputConnection(reader->GetOutputPort());
    imageReslice->SetResliceAxes(transform->GetMatrix());
    imageReslice->SetOutputDimensionality(2);

    vtkNew<vtkPlane> plane;
    plane->SetOrigin(0, 0, 0);
    plane->SetNormal(1, 0, 0);

    vtkNew<vtkImageResliceMapper> imageResliceMapper;
    imageResliceMapper->SetInputConnection(reader->GetOutputPort());
    imageResliceMapper->SetSlicePlane(plane);

    vtkNew<vtkImageSlice> imageSlice;
    imageSlice->SetMapper(imageResliceMapper);
    //imageSlice->GetProperty()->SetOpacity(0.5);

    this->m_rightRenderer->AddActor(imageSlice);

    vtkNew<vtkOutlineFilter> outlineFilter;
    outlineFilter->SetInputConnection(imageResliceMapper->GetOutputPort());

    vtkNew<vtkPolyDataMapper> outlineFilterMapper;
    outlineFilterMapper->SetInputConnection(outlineFilter->GetOutputPort());

    vtkNew<vtkActor> outlineFilterActor;
    outlineFilterActor->SetMapper(outlineFilterMapper);

    this->m_rightRenderer->AddActor(outlineFilterActor);
}
