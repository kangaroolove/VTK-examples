#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkNIFTIImageReader.h>
#include <vtkImageActor.h>
#include <iostream>
#include <vtkMatrix4x4.h>
#include <vtkImageProperty.h>
#include <vtkNIFTIImageWriter.h>
#include <vtkImageData.h>

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
    m_renderer->SetBackground(1.0, 0.0, 0.0);
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData()
{
    // Since VTK8.2.0 vtkImageData doesn't have directionMatrix and the origin is 
    // always (0, 0, 0). I suggest don't use this way to save NIFTII image unless
    // you upgrade the VTK version to 9.0+

    //std::string filePath = "D:/UROPRO/patient/-1/-1/333.nii.gz";
    std::string filePath = "D:/segmentedBlank.nii.gz";
    vtkNew<vtkNIFTIImageReader> reader;
    reader->SetFileName(filePath.data());
    reader->Update();

    double origin[3];
    reader->GetOutput()->GetOrigin(origin);

    std::cout<<"origin[0] = "<<origin[0]<<", origin[1] = "<<origin[1]<<", origin[2] = "<<origin[2]<<endl;

    auto qMatrix = reader->GetQFormMatrix();
    if (qMatrix)
        std::cout<<"qMatrix = "<<*qMatrix<<endl;


    auto sMatrix = reader->GetSFormMatrix();
    if (sMatrix)
        std::cout<<"sMatrix = "<<*sMatrix<<endl;

    std::string outputPath = "D:/writerTest.nii.gz";
    vtkNew<vtkNIFTIImageWriter> writer;
    writer->SetFileName(outputPath.data());
    writer->SetInputData(reader->GetOutput());
    //writer->SetQFormMatrix(reader->GetQFormMatrix());
    writer->Write();

    //const double level = 0.5;

    vtkNew<vtkImageActor> imageActor;
    imageActor->SetInputData(reader->GetOutput());
    // imageActor->GetProperty()->SetColorWindow(level * 2);
    // imageActor->GetProperty()->SetColorLevel(level);
    imageActor->SetDisplayExtent(0, 255, 0, 224, 17, 17);

    double bounds[6];
    imageActor->GetBounds(bounds);

    // m_renderer->AddViewProp(imageActor);
}
