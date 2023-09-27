#include "MarkerAssembly.h"
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkVectorText.h>
#include <vtkProperty.h>
#include <vtkCylinderSource.h>

vtkStandardNewMacro(MarkerAssembly)

void MarkerAssembly::setText(const std::string& text)
{
    m_text = text;
    m_vectorText->SetText(text.c_str());
}

void MarkerAssembly::setTextColor(const double& r, const double& g, const double& b)
{
    m_textActor->GetProperty()->SetColor(r, g, b);
}

void MarkerAssembly::setNormalType(const NormalType &type)
{
    m_normalType = type;

    vtkNew<vtkTransform> transform;
    transform->PostMultiply();
    if (type == NormalType::X)
    {
        transform->RotateY(90);
    }
    else if (type == NormalType::Y)
    {
        transform->RotateX(90);
    }
    transform->Translate(0, 10, 0);
    m_textTransform = transform;
}

MarkerAssembly::MarkerAssembly() :
    m_vectorText(vtkSmartPointer<vtkVectorText>::New()),
    m_text("Marker"),
    m_textActor(vtkSmartPointer<vtkActor>::New()),
    m_normalType(NormalType::X),
    m_textTransform(vtkSmartPointer<vtkTransform>::New())
{
    m_vectorText->SetText(m_text.c_str());

    setNormalType(m_normalType);

    vtkNew<vtkTransformPolyDataFilter> textTransformFilter;
    textTransformFilter->SetInputConnection(m_vectorText->GetOutputPort());
    textTransformFilter->SetTransform(m_textTransform);

    vtkNew<vtkPolyDataMapper> textMapper;
    textMapper->SetInputConnection(textTransformFilter->GetOutputPort());

    m_textActor->SetMapper(textMapper);

    this->AddPart(m_textActor);



    vtkNew<vtkCylinderSource> cylinder;
    cylinder->SetRadius(5);
    cylinder->SetHeight(20);
    cylinder->SetResolution(100);

    vtkNew<vtkTransform> cylinderTransform;
    cylinderTransform->RotateX(90);

    vtkNew<vtkTransformPolyDataFilter> cylinderTransformFilter;
    cylinderTransformFilter->SetInputConnection(cylinder->GetOutputPort());
    cylinderTransformFilter->SetTransform(cylinderTransform);

    vtkNew<vtkPolyDataMapper> cylinderMapper;
    cylinderMapper->SetInputConnection(cylinderTransformFilter->GetOutputPort());

    vtkNew<vtkActor> cylinderActor;
    cylinderActor->SetMapper(cylinderMapper);

    this->AddPart(cylinderActor);
}
