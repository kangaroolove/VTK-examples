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
#include <vtkImageThreshold.h>
#include <itkNiftiImageIO.h>
#include <itkVTKImageToImageFilter.h>
#include <itkImageFileWriter.h>

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
  std::string dir =
      "D:/Standard test-data/Set B - Real Patient/Patient B/CBL_T2";
  int index = 0;
  double level = 883;

  using ImageType = itk::Image<short, 3>;
  using ReaderType = itk::ImageSeriesReader<ImageType>;

  auto itkReader = ReaderType::New();
  using ImageIOType = itk::GDCMImageIO;
  auto dicomIO = ImageIOType::New();

  itkReader->SetImageIO(dicomIO);

  using NamesGeneratorType = itk::GDCMSeriesFileNames;
  auto nameGenerator = NamesGeneratorType::New();

  nameGenerator->SetUseSeriesDetails(true);
  nameGenerator->AddSeriesRestriction("0008|0021");
  nameGenerator->SetDirectory(dir);

  using SeriesIdContainer = std::vector<std::string>;
  const SeriesIdContainer &seriesUID = nameGenerator->GetSeriesUIDs();
  std::string seriesIdentifier;
  seriesIdentifier = seriesUID.begin()->c_str();

  using FileNamesContainer = std::vector<std::string>;
  FileNamesContainer fileNames;
  fileNames = nameGenerator->GetFileNames(seriesIdentifier);
  itkReader->SetFileNames(fileNames);
  try {
    itkReader->Update();
  } catch (...) {
  }

  auto direction = itkReader->GetOutput()->GetDirection();

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
  filter->SetInput(itkReader->GetOutput());
  filter->Update();

  // Get the VTK image
  auto image = filter->GetOutput();
  if (image) {
    qDebug() << "converted successfully";
    // image->Print(std::cout);
  }

  vtkNew<vtkImageThreshold> threshold;
  threshold->SetInputData(image);
  threshold->ThresholdByUpper(1);
  threshold->ReplaceInOn();
  threshold->SetInValue(0);
  threshold->Update();

  saveImage(image, matrix);

  // If I don't use this, the application will crash
  vtkNew<vtkImageReslice> reslice;
  reslice->SetInputData(threshold->GetOutput());
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

  slice->GetProperty()->SetColorWindow(level * 2);
  slice->GetProperty()->SetColorLevel(level);
  slice->SetUserMatrix(matrix);

  m_renderer->AddViewProp(slice);
  m_style->SetImageSlice(slice);

  m_renderer->GetActiveCamera()->ApplyTransform(transform);
  m_renderer->ResetCamera();
}

void VTKOpenGLWidget::saveImage(vtkImageData *image,
                                vtkMatrix4x4 *directionMatrix) {
                                    using ImageType = itk::Image<short, 3>;
  using VTKImageToImageType = itk::VTKImageToImageFilter<ImageType>;
  auto vtkImageToImageFilter = VTKImageToImageType::New();
  vtkImageToImageFilter->SetInput(image);
  vtkImageToImageFilter->Update();

  auto itkImage = vtkImageToImageFilter->GetOutput();

  itk::Matrix<double, 3, 3> itkMatrix;

  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      itkMatrix(i, j) = directionMatrix->GetElement(i, j);

  itkImage->SetDirection(itkMatrix);

  using WriterType = itk::ImageFileWriter<ImageType>;
  std::string fileName = "D:/ttttt.nii.gz";
  auto niftiIO = itk::NiftiImageIO::New();

  auto writer = WriterType::New();
  writer->SetFileName(fileName.data());
  writer->SetInput(itkImage);
  writer->SetImageIO(niftiIO);
  writer->Update();
                                }
