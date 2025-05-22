#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <array>
#include <vtkSmartPointer.h>

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget {
public:
    VTKOpenGLWidget(QWidget *parent = nullptr);
    ~VTKOpenGLWidget();

private:
    void initialize();
    void createTestData();
    void createAnnotation();

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    std::array<vtkSmartPointer<vtkRenderer>, 3> m_renderers;
};