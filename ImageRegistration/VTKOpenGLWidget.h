#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class vtkImageData;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget {
public:
    enum SliceOrientation {
        Sagittal = 0,   // YZ plane, slice along X
        Coronal = 1,    // XZ plane, slice along Y
        Transverse = 2  // XY plane, slice along Z
    };

    VTKOpenGLWidget(QWidget *parent = nullptr);

private:
    void initialize();
    void createTestData();
    void setupSliceView(vtkRenderer *renderer, vtkImageData *image, int orientation);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_transverseRenderer;
    vtkSmartPointer<vtkRenderer> m_sagittalRenderer;
    vtkSmartPointer<vtkRenderer> m_coronalRenderer;
};
