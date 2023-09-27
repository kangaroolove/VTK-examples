#include "MarkerAssembly.h"
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkVectorText.h>

vtkStandardNewMacro(MarkerAssembly)

void MarkerAssembly::setText(const std::string& text)
{
    m_text = text;
    m_vectorText->SetText(text.c_str());
}

MarkerAssembly::MarkerAssembly() :
    m_vectorText(vtkSmartPointer<vtkVectorText>::New()),
    m_text("Marker")
{

    m_vectorText->SetText(m_text.c_str());

    vtkNew<vtkTransform> textTransform;
    textTransform->Translate(0, 10, 0);

    vtkNew<vtkTransformPolyDataFilter> textTransformFilter;
    textTransformFilter->SetInputConnection(m_vectorText->GetOutputPort());
    textTransformFilter->SetTransform(textTransform);

    vtkNew<vtkPolyDataMapper> textMapper;
    textMapper->SetInputConnection(textTransformFilter->GetOutputPort());

    vtkNew<vtkActor> textActor;
    textActor->SetMapper(textMapper);


    this->AddPart(textActor);



    // vtkNew<vtkCylinderSource> cylinder;
    // cylinder->SetRadius(5);
    // cylinder->SetHeight(20);
    // cylinder->SetResolution(100);

    // vtkNew<vtkTransform> cylinderTransform;
    // cylinderTransform->RotateX(90);

    // vtkNew<vtkTransformPolyDataFilter> cylinderTransformFilter;
    // cylinderTransformFilter->SetInputConnection(cylinder->GetOutputPort());
    // cylinderTransformFilter->SetTransform(cylinderTransform);

    // vtkNew<vtkPolyDataMapper> cylinderMapper;
    // cylinderMapper->SetInputConnection(cylinderTransformFilter->GetOutputPort());

    // vtkNew<vtkActor> cylinderActor;
    // cylinderActor->SetMapper(cylinderMapper);
}
