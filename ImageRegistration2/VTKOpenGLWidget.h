#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <itkImage.h>
#include <itkPoint.h>
#include <vtkSmartPointer.h>

#include <string>

class vtkGenericOpenGLRenderWindow;
class vtkImageData;
class vtkImageSlice;
class vtkMatrix4x4;
class vtkPlane;
class vtkRenderer;
class KeyPressInteractorStyle;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget {
public:
    VTKOpenGLWidget(QWidget *parent = nullptr);
    ~VTKOpenGLWidget();

    void runRegistration();
    void runRegistrationWithCenters();

    void setUsOpacity(double opacity);
    void setMriOpacity(double opacity);

    // Converts IJK voxel indices to LPS world coordinates for both images.
    // usWorld and mriWorld are the output physical points.
    void imageIJKToWorld(const itk::Index<3> &usIJK, const itk::Index<3> &mriIJK,
                         itk::Point<double, 3> &usWorld,
                         itk::Point<double, 3> &mriWorld) const;

private:
    static itk::Point<double, 3> itkIJKToWorld(const itk::Image<short, 3> *image,
                                                const itk::Index<3> &ijk);
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
    vtkSmartPointer<vtkImageSlice> m_usSlice;
    vtkSmartPointer<vtkImageSlice> m_mriSlice;
    vtkSmartPointer<vtkPlane> m_plane;

    itk::Image<short, 3>::Pointer m_usItkImage;
    itk::Image<short, 3>::Pointer m_mriItkImage;

    std::string m_mriDir;
    std::string m_ultrasoundPath;
};
