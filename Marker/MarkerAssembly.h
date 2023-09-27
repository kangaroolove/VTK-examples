#pragma once

#include <vtkAssembly.h>
#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>

class vtkVectorText;
class vtkActor;
class vtkCutter;
class vtkPlane;

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
    void setColor(const double& r, const double& g, const double& b);
    void setNormalType(const NormalType& type);
protected:
    MarkerAssembly();

    std::string m_text;
    vtkSmartPointer<vtkVectorText> m_vectorText;
    vtkSmartPointer<vtkActor> m_textActor;
    NormalType m_normalType;
    vtkSmartPointer<vtkTransform> m_textTransform;
    vtkSmartPointer<vtkCutter> m_cutter;
    vtkSmartPointer<vtkPlane> m_plane;
    vtkSmartPointer<vtkActor> m_cutterActor;
};