#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class KeyPressInteractorStyle;
class vtkImageData;
class vtkMatrix4x4;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget {
public:
  VTKOpenGLWidget(QWidget *parent = nullptr);
  ~VTKOpenGLWidget();

private:
  void initialize();
  void createTestData();
  void saveImage(vtkImageData *image, vtkMatrix4x4 *directionMatrix);

  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
  vtkSmartPointer<vtkRenderer> m_renderer;
  vtkSmartPointer<KeyPressInteractorStyle> m_style;
};