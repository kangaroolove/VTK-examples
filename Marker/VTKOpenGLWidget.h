#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <vtkCaptionActor2D.h>
#include <vtkSmartPointer.h>

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class KeyPressInteractorStyle;
class MarkerAssembly;
class vtkTransform;
class MarkerPointActor;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget {
public:
    VTKOpenGLWidget(QWidget *parent = nullptr);
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
    vtkSmartPointer<MarkerPointActor> m_markerActor;
};