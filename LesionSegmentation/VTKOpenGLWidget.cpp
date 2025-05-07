#include "VTKOpenGLWidget.h"
#include <QDebug>
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImage.h>
#include <itkImageSeriesReader.h>
#include <itkImageToVTKImageFilter.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkConeSource.h>
#include <vtkDICOMImageReader.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkImageProperty.h>
#include <vtkImageReslice.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <itkNiftiImageIO.h>
#include <itkImageFileReader.h>

class KeyPressInteractorStyle : public vtkInteractorStyleTrackballCamera {
public:
  static KeyPressInteractorStyle *New();
  vtkTypeMacro(KeyPressInteractorStyle, vtkInteractorStyleTrackballCamera);

  virtual void OnKeyPress() override {
    vtkRenderWindowInteractor *rwi = this->Interactor;
    std::string key = rwi->GetKeySym();

    std::cout << "Pressed " << key << endl;
    if (key == "Up") {
      // double bounds[6];
      // mImageActor->GetBounds(bounds);
      // for (int i = 0; i < 6; i++)
      // {
      //     qDebug()<<"bounds["<<i<<"] = "<<bounds[i];
      // }
    } else if (key == "Down") {
      double bounds[6];
      mImageSlice->GetBounds(bounds);
      for (int i = 0; i < 6; i++) {
        qDebug() << "bounds[" << i << "] = " << bounds[i];
      }
    } else if (key == "Left") {
      auto camera = mRenderer->GetActiveCamera();
      double position[3];
      camera->GetPosition(position);
      qDebug() << "Camera position";
      for (int i = 0; i < 3; ++i)
        qDebug() << position[i];

      double focalPoint[3];
      camera->GetFocalPoint(focalPoint);
      qDebug() << "Camera focalPoint";
      for (int i = 0; i < 3; ++i)
        qDebug() << focalPoint[i];
    } else if (key == "Right") {
    }

    vtkInteractorStyleTrackballCamera::OnKeyPress();
  }

  void SetImageSlice(vtkImageSlice *slice) { mImageSlice = slice; }

  void SetRenderWindow(vtkRenderWindow *window) { mRenderWindow = window; }

  void SetRenderer(vtkRenderer *renderer) { mRenderer = renderer; }

private:
  vtkImageActor *mImageActor;
  vtkImageSlice *mImageSlice;
  vtkRenderWindow *mRenderWindow;
  vtkRenderer *mRenderer;
};
vtkStandardNewMacro(KeyPressInteractorStyle);

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_renderer(vtkSmartPointer<vtkRenderer>::New()),
      m_style(vtkSmartPointer<KeyPressInteractorStyle>::New()) {
  initialize();
  createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
  m_renderer->SetBackground(0.0, 1.0, 0.0);
  m_renderWindow->AddRenderer(m_renderer);
  SetRenderWindow(m_renderWindow);
  m_renderWindow->GetInteractor()->SetInteractorStyle(m_style);
  m_style->SetRenderer(m_renderer);
}

void VTKOpenGLWidget::createTestData() {
  QString inputFileName = "D:/test4_1_lesion.nii.gz";

  using ImageType = itk::Image<short, 3>;
  using ReaderType = itk::ImageFileReader<ImageType>;

  auto niftiIO = itk::NiftiImageIO::New();
  auto reader = ReaderType::New();
  reader->SetFileName(inputFileName.toStdString().data());
  reader->SetImageIO(niftiIO);

  try {
    reader->Update();
  } catch (itk::ExceptionObject &error) {
    qCritical() << QString("ITK image read with error %1").arg(error.what());
    return;
  }
  qDebug() << "Read " << inputFileName << " file successfully";

  auto direction = reader->GetOutput()->GetDirection();

  // Get direction matrix
  vtkNew<vtkMatrix4x4> matrix;
  matrix->Identity();

  for (unsigned int i = 0; i < 3; ++i) {
    for (unsigned int j = 0; j < 3; ++j) {
      matrix->SetElement(i, j, direction(i, j));
    }
  }

  using FilterType = itk::ImageToVTKImageFilter<ImageType>;
  FilterType::Pointer filter = FilterType::New();
  filter->SetInput(reader->GetOutput());
  filter->Update();

  // Get the VTK image
  auto image = filter->GetOutput();
  if (image) {
    qDebug() << "converted successfully";
    // image->Print(std::cout);
  }


  vtkNew<vtkImageReslice> reslice;
  reslice->SetInputData(filter->GetOutput());
  reslice->Update();

  reslice->GetOutput()->Print(std::cout);

  vtkNew<vtkTransform> transform;
  transform->SetMatrix(matrix);

  vtkNew<vtkImageResliceMapper> mapper;
  mapper->SetInputData(reslice->GetOutput());

  // qDebug()<<"mapper->GetSliceAtFocalPoint() =
  // "<<mapper->GetSliceAtFocalPoint(); qDebug()<<"mapper->SetSliceFacesCamera()
  // = "<<mapper->GetSliceFacesCamera();

  mapper->SetSliceAtFocalPoint(true);
  mapper->SetSliceFacesCamera(true);

  vtkNew<vtkImageSlice> slice;
  slice->SetMapper(mapper);

  // slice->GetProperty()->SetColorWindow(level * 2);
  // slice->GetProperty()->SetColorLevel(level);
  slice->SetUserMatrix(matrix);

  m_renderer->AddViewProp(slice);
  m_style->SetImageSlice(slice);

  m_renderer->GetActiveCamera()->ApplyTransform(transform);
  m_renderer->ResetCamera();
}
