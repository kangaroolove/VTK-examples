#include "VTKOpenGLWidget.h"
#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkDICOMImageReader.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkImageStack.h>
#include <vtkNew.h>
#include <vtkNrrdReader.h>
#include <vtkPlane.h>
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
  std::vector<vtkImageStack *> stacks = {
      vtkImageStack::New(), vtkImageStack::New(), vtkImageStack::New()};
  for (int i = 0; i < 3; i++) {
    m_renderer->AddActor(stacks[i]);
  }

  double origin[3] = {10, 50, 0};

  vtkNew<vtkDICOMImageReader> dicomReader;
  dicomReader->SetDirectoryName("D:/Real-Patient-Data/Patient A/t2");

  for (int i = 0; i < 3; i++) {
    double normal[3] = {0};
    normal[i] = 1;
    vtkNew<vtkPlane> plane;
    plane->SetOrigin(origin);
    plane->SetNormal(normal);

    vtkNew<vtkImageResliceMapper> mapper;
    mapper->SetInputConnection(dicomReader->GetOutputPort());
    mapper->SetSlicePlane(plane);

    vtkNew<vtkImageSlice> slice;
    slice->SetMapper(mapper);

    stacks[i]->AddImage(slice);
  }

  vtkNew<vtkNrrdReader> nrrdReader;
  nrrdReader->SetFileName("D:/MRI.nrrd");

  for (int i = 0; i < 3; i++) {
    double normal[3] = {0};
    normal[i] = 1;
    vtkNew<vtkPlane> plane;
    plane->SetOrigin(origin);
    plane->SetNormal(normal);

    vtkNew<vtkImageResliceMapper> mapper;
    mapper->SetInputConnection(nrrdReader->GetOutputPort());
    mapper->SetSlicePlane(plane);

    vtkNew<vtkImageSlice> slice;
    slice->SetMapper(mapper);

    stacks[i]->AddImage(slice);
  }
}
