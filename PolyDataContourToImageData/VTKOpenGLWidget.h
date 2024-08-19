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

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget
{
public:
    VTKOpenGLWidget(QWidget* parent = nullptr);
    ~VTKOpenGLWidget();

private:
    void initialize();
    void createTestData();
    void initColor(vtkImageData* image, const int& color);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<InteractorStyleImage> m_interactorStyle;
    vtkSmartPointer<vtkPoints> m_linePoints;
    vtkSmartPointer<vtkCellArray> m_lineCells;
    vtkSmartPointer<vtkPolyData> m_lineData;
};