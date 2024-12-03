#include "VTKOpenGLWidget.h"
#include <QDebug>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageToVTKImageFilter.h>
#include <itkNiftiImageIO.h>
#include <itkVTKImageToImageFilter.h>
#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_renderer(vtkSmartPointer<vtkRenderer>::New()) {
  initialize();
  createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
  m_renderWindow->AddRenderer(m_renderer);
  SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData() {

  QString inputFileName = "D:/lesion1A.nii.gz";
  QString outputFileName = "D:/testNiiSave.nii.gz";

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

  using WriterType = itk::ImageFileWriter<ImageType>;
  auto writer = WriterType::New();
  writer->SetFileName(outputFileName.toStdString().data());
  writer->SetInput(reader->GetOutput());

  try {
    writer->Update();
  } catch (itk::ExceptionObject &error) {
    qCritical() << QString("ITK image write with error %1").arg(error.what());
    return;
  }
  qDebug() << "Write" << outputFileName << " file successfully";
}
