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

    void calculateWorldPositionFromImageIJK(vtkImageData *image,
                                            vtkMatrix4x4 *directionMatrix,
                                            int in[3], double out[3]);
    void calculateImageIJKFromWorldPosition(vtkImageData *image,
                                            vtkMatrix4x4 *directionMatrix,
                                            double in[3], int out[3]);

    void printArray(const std::string &name, double array[3]);
    void saveImage(vtkImageData *image, vtkMatrix4x4 *directionMatrix);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<KeyPressInteractorStyle> m_style;
};