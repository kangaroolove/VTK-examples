#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>
#include <vtkCaptionActor2D.h>

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class KeyPressInteractorStyle;
class MarkerAssembly;
class vtkTransform;
class MarkerActor;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget
{
public:
    VTKOpenGLWidget(QWidget* parent = nullptr);
    ~VTKOpenGLWidget();

private:
    void initialize();
    void createTestData();

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<KeyPressInteractorStyle> m_style;
    vtkSmartPointer<MarkerAssembly> m_markerAssembly;
    vtkSmartPointer<vtkCaptionActor2D> m_captionActor;
    vtkSmartPointer<vtkTransform> m_transform;
    vtkSmartPointer<MarkerActor> m_markerActor;
};