#include "VTKOpenGLWidget.h"

#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageSeriesReader.h>
#include <itkImageToVTKImageFilter.h>
#include <itkNiftiImageIO.h>
#include <itkVTKImageToImageFilter.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkConeSource.h>
#include <vtkDICOMImageReader.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageConnectivityFilter.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkImageProperty.h>
#include <vtkImageReslice.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkImageStencil.h>
#include <vtkImageThreshold.h>
#include <vtkImageThresholdConnectivity.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMatrix4x4.h>
#include <vtkNIFTIImageWriter.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkROIStencilSource.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

#include <QDebug>

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
          for (int i = 0; i < 3; ++i) qDebug() << position[i];

          double focalPoint[3];
          camera->GetFocalPoint(focalPoint);
          qDebug() << "Camera focalPoint";
          for (int i = 0; i < 3; ++i) qDebug() << focalPoint[i];
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

bool VTKOpenGLWidget::readNiftiImage(const QString &fileName,
                                     vtkImageData *image,
                                     vtkMatrix4x4 *matrix) {
    using ImageType = itk::Image<short, 3>;
    using ReaderType = itk::ImageFileReader<ImageType>;

    auto niftiIO = itk::NiftiImageIO::New();
    auto reader = ReaderType::New();
    reader->SetFileName(fileName.toStdString().data());
    reader->SetImageIO(niftiIO);

    try {
        reader->Update();
    } catch (itk::ExceptionObject &error) {
        qCritical()
            << QString("ITK image read with error %1").arg(error.what());
        return false;
    }
    qDebug() << "Read " << fileName << " file successfully";

    auto direction = reader->GetOutput()->GetDirection();
    matrix->Identity();
    for (unsigned int i = 0; i < 3; ++i)
        for (unsigned int j = 0; j < 3; ++j)
            matrix->SetElement(i, j, direction(i, j));

    using FilterType = itk::ImageToVTKImageFilter<ImageType>;
    auto filter = FilterType::New();
    filter->SetInput(reader->GetOutput());
    filter->Update();

    image->DeepCopy(filter->GetOutput());
    qDebug() << "converted successfully";
    return true;
}

void VTKOpenGLWidget::createTestData() {
    QString inputFileName = "D:/test4_1_lesion.nii.gz";

    vtkNew<vtkMatrix4x4> matrix;
    vtkNew<vtkImageData> imageData;
    if (!readNiftiImage(inputFileName, imageData, matrix)) return;

    auto image = imageData.Get();

    vtkNew<vtkImageConnectivityFilter> connectivityFilter;
    connectivityFilter->SetInputData(image);
    connectivityFilter->SetScalarRange(1, 1);
    connectivityFilter->SetExtractionModeToAllRegions();
    connectivityFilter->GenerateRegionExtentsOn();
    connectivityFilter->Update();

    const int EXTENT_SIZE = 6;

    vtkIdTypeArray *sizeArray = connectivityFilter->GetExtractedRegionSizes();
    vtkIdTypeArray *idArray = connectivityFilter->GetExtractedRegionSeedIds();
    vtkIdTypeArray *labelArray = connectivityFilter->GetExtractedRegionLabels();
    vtkIntArray *extentArray = connectivityFilter->GetExtractedRegionExtents();
    vtkIdType rn = connectivityFilter->GetNumberOfExtractedRegions();

    std::cout << "number of regions: " << rn << std::endl;
    for (vtkIdType r = 0; r < rn; r++) {
        std::cout << "region: " << r << ","
                  << " seed: " << idArray->GetValue(r) << ","
                  << " label: " << labelArray->GetValue(r) << ","
                  << " size: " << sizeArray->GetValue(r) << ","
                  << " extent: [";

        if (connectivityFilter->GetGenerateRegionExtents()) {
            std::array<int, 6> region;
            for (int i = 0; i < EXTENT_SIZE; ++i)
                region[i] = extentArray->GetValue(EXTENT_SIZE * r + i);

            std::cout << region[0] << "," << region[1] << "," << region[2]
                      << "," << region[3] << "," << region[4] << ","
                      << region[5] << endl;

            double origin[3], spacing[3];
            image->GetOrigin(origin);
            image->GetSpacing(spacing);

            vtkNew<vtkROIStencilSource> stencilSource;
            stencilSource->SetInformationInput(image);
            stencilSource->SetBounds(origin[0] + region[0] * spacing[0],
                                     origin[0] + region[1] * spacing[0],
                                     origin[1] + region[2] * spacing[1],
                                     origin[1] + region[3] * spacing[1],
                                     origin[2] + region[4] * spacing[2],
                                     origin[2] + region[5] * spacing[2]);
            stencilSource->Update();

            vtkNew<vtkImageStencil> stencil;
            stencil->SetInputData(image);
            stencil->SetStencilConnection(stencilSource->GetOutputPort());
            stencil->SetBackgroundValue(0);
            stencil->ReverseStencilOff();
            stencil->Update();

            saveImage(stencil->GetOutput(), matrix,
                      QString("D:/lesionSegmentation_%1.nii.gz").arg(r));
        }
        std::cout << "]" << std::endl;
    }

    vtkNew<vtkImageReslice> reslice;
    reslice->SetInputData(imageData);
    reslice->Update();

    // reslice->GetOutput()->Print(std::cout);

    vtkNew<vtkTransform> transform;
    transform->SetMatrix(matrix);

    vtkNew<vtkImageResliceMapper> mapper;
    mapper->SetInputData(reslice->GetOutput());
    mapper->SetSliceAtFocalPoint(true);
    mapper->SetSliceFacesCamera(true);

    vtkNew<vtkImageSlice> slice;
    slice->SetMapper(mapper);
    slice->SetUserMatrix(matrix);

    m_renderer->AddViewProp(slice);
    m_style->SetImageSlice(slice);

    m_renderer->GetActiveCamera()->ApplyTransform(transform);
    m_renderer->ResetCamera();
}

void VTKOpenGLWidget::saveImage(vtkImageData *image,
                                vtkMatrix4x4 *directionMatrix,
                                const QString &fileName) {
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

    using Writer1Type = itk::ImageFileWriter<ImageType>;
    auto niftiIO = itk::NiftiImageIO::New();

    auto writer = Writer1Type::New();
    writer->SetFileName(fileName.toStdString().data());
    writer->SetInput(itkImage);
    writer->SetImageIO(niftiIO);
    writer->Update();
}
