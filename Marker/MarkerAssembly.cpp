#include "MarkerAssembly.h"
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkVectorText.h>
#include <vtkProperty.h>
#include <vtkCylinderSource.h>
#include <vtkCutter.h>
#include <vtkPlane.h>

vtkStandardNewMacro(MarkerAssembly)

void MarkerAssembly::setText(const std::string& text)
{
    m_text = text;
    m_vectorText->SetText(text.c_str());
}

void MarkerAssembly::setTextVisible(const bool &visible)
{
    m_textActor->SetVisibility(visible);
}

void MarkerAssembly::setColor(const double& r, const double& g, const double& b)
{
    m_textActor->GetProperty()->SetColor(r, g, b);
    m_cutterActor->GetProperty()->SetColor(r, g, b);
}

void MarkerAssembly::setNormalType(const NormalType &type)
{
    m_normalType = type;

    vtkNew<vtkTransform> transform;
    transform->PostMultiply();
    if (type == NormalType::X)
    {
        m_plane->SetNormal(1, 0, 0);
        transform->RotateY(90);
    }
    else if (type == NormalType::Y)
    {
        m_plane->SetNormal(0, 1, 0);
        transform->RotateX(90);
    }
    else if (type == NormalType::Z)
        m_plane->SetNormal(0, 0, 1);

    transform->Translate(0, 10, 0);
    m_textTransform = transform;
}

MarkerAssembly::MarkerAssembly() :
    m_vectorText(vtkSmartPointer<vtkVectorText>::New()),
    m_text("Marker"),
    m_textActor(vtkSmartPointer<vtkActor>::New()),
    m_normalType(NormalType::Y),
    m_textTransform(vtkSmartPointer<vtkTransform>::New()),
    m_cutter(vtkSmartPointer<vtkCutter>::New()),
    m_plane(vtkSmartPointer<vtkPlane>::New()),
    m_cutterActor(vtkSmartPointer<vtkActor>::New())
{
    m_plane->SetOrigin(0, 0, 0);
    setNormalType(m_normalType);

    m_vectorText->SetText(m_text.c_str());

    vtkNew<vtkTransformPolyDataFilter> textTransformFilter;
    textTransformFilter->SetInputConnection(m_vectorText->GetOutputPort());
    textTransformFilter->SetTransform(m_textTransform);

    vtkNew<vtkPolyDataMapper> textMapper;
    textMapper->SetInputConnection(textTransformFilter->GetOutputPort());

    m_textActor->SetMapper(textMapper);

    this->AddPart(m_textActor);

    
    setTextVisible(false);





    vtkNew<vtkCylinderSource> cylinder;
    cylinder->SetRadius(5);
    cylinder->SetHeight(20);
    cylinder->SetResolution(100);

    vtkNew<vtkTransform> cylinderTransform;
    cylinderTransform->RotateX(90);

    vtkNew<vtkTransformPolyDataFilter> cylinderTransformFilter;
    cylinderTransformFilter->SetInputConnection(cylinder->GetOutputPort());
    cylinderTransformFilter->SetTransform(cylinderTransform);

    m_cutter->SetCutFunction(m_plane);
    m_cutter->SetInputConnection(cylinderTransformFilter->GetOutputPort());

    vtkNew<vtkPolyDataMapper> cutterMapper;
    cutterMapper->SetInputConnection(m_cutter->GetOutputPort());

    m_cutterActor->SetMapper(cutterMapper);

    this->AddPart(m_cutterActor);
}
