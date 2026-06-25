#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <itkImage.h>
#include <vtkSmartPointer.h>

#include <string>

class vtkGenericOpenGLRenderWindow;
class vtkImageData;
class vtkImageSlice;
class vtkMatrix4x4;
class vtkRenderer;
class KeyPressInteractorStyle;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget {
public:
    VTKOpenGLWidget(QWidget *parent = nullptr);
    ~VTKOpenGLWidget();

    void runRegistration();

private:
    void initialize();
    void createTestData();
    vtkSmartPointer<vtkImageData> loadDICOMImage(const std::string &dir,
                                                  vtkSmartPointer<vtkMatrix4x4> &outMatrix,
                                                  itk::Image<short, 3>::Pointer &outItkImage);
    vtkSmartPointer<vtkImageData> loadNrrdImage(const std::string &dir,
                                                 vtkSmartPointer<vtkMatrix4x4> &outMatrix,
                                                 itk::Image<short, 3>::Pointer &outItkImage);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<KeyPressInteractorStyle> m_style;
    vtkSmartPointer<vtkImageSlice> m_mriSlice;

    itk::Image<short, 3>::Pointer m_usItkImage;
    itk::Image<short, 3>::Pointer m_mriItkImage;
};
