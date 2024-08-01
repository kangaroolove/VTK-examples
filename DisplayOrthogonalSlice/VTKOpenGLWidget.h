#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>
#include <array>

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget
{
public:
    VTKOpenGLWidget(QWidget* parent = nullptr);
    ~VTKOpenGLWidget();

private:
    void initialize();
    void createTestData();
    void createAnnotation();

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    std::array<vtkSmartPointer<vtkRenderer>, 3> m_renderers;
};