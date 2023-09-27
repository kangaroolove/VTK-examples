#pragma once

#include <vtkAssembly.h>
#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>

class vtkVectorText;
class vtkActor;

class MarkerAssembly : public vtkAssembly
{
public:
    enum NormalType
    {
        X,
        Y,
        Z
    };

    vtkTypeMacro(MarkerAssembly, vtkAssembly);
    static MarkerAssembly* New();

    void setText(const std::string& text);
    void setTextColor(const double& r, const double& g, const double& b);
protected:
    MarkerAssembly();

    std::string m_text;
    vtkSmartPointer<vtkVectorText> m_vectorText;
    vtkSmartPointer<vtkActor> m_textActor;
    NormalType m_normalType;
};