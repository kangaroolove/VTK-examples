#pragma once

#include <vtkAssembly.h>
#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>

class vtkVectorText;

class MarkerAssembly : public vtkAssembly
{
public:
    vtkTypeMacro(MarkerAssembly, vtkAssembly);
    static MarkerAssembly* New();

    
    void setText(const std::string& text);
protected:
    MarkerAssembly();

    std::string m_text;
    vtkSmartPointer<vtkVectorText> m_vectorText;
};