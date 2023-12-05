#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkNrrdReader.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkOutlineFilter.h>
#include <vtkActor.h>

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
    vtkNew<vtkNrrdReader> reader;
    reader->SetFileName("D:/MRI.nrrd");

    vtkNew<vtkImageSliceMapper> mapper;
    mapper->SetInputConnection(reader->GetOutputPort());

    vtkNew<vtkImageSlice> imageSlice;
    imageSlice->SetMapper(mapper);

    vtkNew<vtkOutlineFilter> outlineFilter;
    outlineFilter->SetInputConnection(reader->GetOutputPort());

    vtkNew<vtkPolyDataMapper> outlineFilterMapper;
    outlineFilterMapper->SetInputConnection(outlineFilter->GetOutputPort());

    vtkNew<vtkActor> outlineActor;
    outlineActor->SetMapper(outlineFilterMapper);

    m_renderer->AddActor(imageSlice);
    m_renderer->AddActor(outlineActor);
}
