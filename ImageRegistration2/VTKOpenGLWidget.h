#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>

#include <string>

class vtkGenericOpenGLRenderWindow;
class vtkImageData;
class vtkMatrix4x4;
class vtkRenderer;
class KeyPressInteractorStyle;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget {
public:
    VTKOpenGLWidget(QWidget *parent = nullptr);
    ~VTKOpenGLWidget();

private:
    void initialize();
    void createTestData();
    vtkSmartPointer<vtkImageData> loadDICOMImage(const std::string &dir,
                                                  vtkSmartPointer<vtkMatrix4x4> &outMatrix);
    vtkSmartPointer<vtkImageData> loadNrrdImage(
        const std::string &dir, vtkSmartPointer<vtkMatrix4x4> &outMatrix);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<KeyPressInteractorStyle> m_style;
};