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
#include <vtkTransformFilter.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_leftRenderer(vtkSmartPointer<vtkRenderer>::New())
    , m_rightRenderer(vtkSmartPointer<vtkRenderer>::New())
    , m_origin{50, 50, 0}
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
    createLeftRenderData();
    createRightRenderData();
}

void VTKOpenGLWidget::createLeftRenderData()
{
    vtkNew<vtkNrrdReader> reader;
    reader->SetFileName("D:/MRI.nrrd");

    for (int i = 0; i < 1; ++i)
    {
        vtkNew<vtkTransform> transform;
        if (i == 0)
        {
            transform->RotateZ(10);
        }
        if (i == 1)
        {
            transform->RotateY(90);
        }
        else if (i == 2)
            transform->RotateX(90);

        vtkNew<vtkMatrix4x4> matrix;
        matrix->DeepCopy(transform->GetMatrix());

        for (int j = 0; j < 3; ++j)
            matrix->SetElement(j, 3, m_origin[j]);

        vtkNew<vtkImageReslice> imageReslice;
        imageReslice->SetInputConnection(reader->GetOutputPort());
        imageReslice->SetResliceAxes(matrix);
        //imageReslice->SetResliceTransform(transform);
        imageReslice->SetOutputDimensionality(2);

        // vtkNew<vtkTransformFilter> transformFilter;
        // transformFilter->SetInputConnection(imageReslice->GetOutputPort());
        // transformFilter->SetTransform(transform);

        // vtkNew<vtkPolyDataMapper> mapper;
        // mapper->SetInputConnection(transformFilter->GetOutputPort());

        // vtkNew<vtkActor> actor;
        // actor->SetMapper(mapper);

        //this->m_leftRenderer->AddActor(actor);

        vtkNew<vtkImageResliceMapper> vtkImageResliceMapper;
        vtkImageResliceMapper->SetInputConnection(imageReslice->GetOutputPort());

        vtkImageResliceMapper->GetOutputPort();
        //vtkMatrix4x4* newMatrix = vtkImageResliceMapper->GetDataToWorldMatrix();

        vtkNew<vtkImageSlice> imageSlice;
        imageSlice->SetMapper(vtkImageResliceMapper);

        this->m_leftRenderer->AddActor(imageSlice);

            vtkNew<vtkOutlineFilter> outlineFilter;
    outlineFilter->SetInputConnection(vtkImageResliceMapper->GetOutputPort());

    vtkNew<vtkPolyDataMapper> outlineFilterMapper;
    outlineFilterMapper->SetInputConnection(outlineFilter->GetOutputPort());

    vtkNew<vtkActor> outlineFilterActor;
    outlineFilterActor->SetMapper(outlineFilterMapper);

    this->m_leftRenderer->AddActor(outlineFilterActor);
    }

#if 0
    vtkNew<vtkOutlineFilter> outlineFilter;
    outlineFilter->SetInputConnection(imageResliceMapper->GetOutputPort());

    vtkNew<vtkPolyDataMapper> outlineFilterMapper;
    outlineFilterMapper->SetInputConnection(outlineFilter->GetOutputPort());

    vtkNew<vtkActor> outlineFilterActor;
    outlineFilterActor->SetMapper(outlineFilterMapper);

    this->m_rightRenderer->AddActor(outlineFilterActor);
#endif
}

void VTKOpenGLWidget::createRightRenderData()
{
    vtkNew<vtkNrrdReader> reader;
    reader->SetFileName("D:/MRI.nrrd");

    for (int i = 0; i < 3; ++i)
    {
        vtkNew<vtkPlane> plane;
        plane->SetOrigin(m_origin);
        double normal[3] = { 0 };
        normal[i] = 1;
        plane->SetNormal(normal);

        vtkNew<vtkImageResliceMapper> imageResliceMapper;
        imageResliceMapper->SetInputConnection(reader->GetOutputPort());
        imageResliceMapper->SetSlicePlane(plane);

        vtkNew<vtkImageSlice> imageSlice;
        imageSlice->SetMapper(imageResliceMapper);

        this->m_rightRenderer->AddActor(imageSlice);
    }
}
