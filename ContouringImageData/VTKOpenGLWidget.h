#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class InteractorStyleImage;
class vtkPoints;
class vtkCellArray;
class vtkPolyData;
class vtkImageData;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget {
public:
    VTKOpenGLWidget(QWidget *parent = nullptr);
    void saveImageToLocal();
    void setEraseOn(bool on);
    void autoFill();

private:
    void initialize();
    void createTestData();
    void initColor(vtkImageData *image, const int &color);
    std::vector<std::array<int, 6>> detectImageHole();

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<InteractorStyleImage> m_interactorStyle;
    vtkImageData *m_baseImage;
};