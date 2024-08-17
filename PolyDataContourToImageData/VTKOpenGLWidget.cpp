#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkSphereSource.h>
#include <vtkCutter.h>
#include <vtkPlane.h>
#include <vtkStripper.h>
#include <vtkImageData.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkImageStencil.h>
#include <vtkPointData.h>
#include <vtkImageActor.h>
#include <vtkInteractorStyleImage.h>
#include <QDebug>
#include <vtkCoordinate.h>

class InteractorStyleImage : public vtkInteractorStyleImage
{
public:
  static InteractorStyleImage* New();
  vtkTypeMacro(InteractorStyleImage, vtkInteractorStyleImage);

  void OnLeftButtonDown() override
  {
    vtkInteractorStyleImage::OnLeftButtonDown();
    m_leftButtonPress = true;
  }

  void OnLeftButtonUp() override
  {
    vtkInteractorStyleImage::OnLeftButtonUp();
    m_leftButtonPress = false;
  } 

  void OnMouseMove() override
  {
    vtkInteractorStyleImage::OnMouseMove();
    int eventPosition[2] = { 0 };
    this->GetInteractor()->GetEventPosition(eventPosition);

    if (!m_leftButtonPress)
      return;

    double distance = ((m_lastEventPosition[0] - eventPosition[0]) * (m_lastEventPosition[0] - eventPosition[0])) + ((m_lastEventPosition[1] - eventPosition[1]) * (m_lastEventPosition[1] - eventPosition[1]));
    qDebug()<<"distance = "<<distance;
    if (distance > 0.0)
    {
      double* worldPosition = nullptr;
      vtkNew<vtkCoordinate> coordinate;
      coordinate->SetCoordinateSystemToDisplay();
      coordinate->SetValue(eventPosition[0], eventPosition[1], 0);
      worldPosition = coordinate->GetComputedWorldValue(m_renderer);
      for (int i  = 0; i < 3; i++)
      {
        qDebug()<<"x = "<<worldPosition[0];
        qDebug()<<"y = "<<worldPosition[1];
        qDebug()<<"z = "<<worldPosition[2];
      }
    }

    m_lastEventPosition[0] = eventPosition[0];
    m_lastEventPosition[1] = eventPosition[1];
  }

  void setContouringImage(vtkImageData* image)
  {
    m_contouringImage = image;
  }

  void setRenderer(vtkRenderer* renderer)
  {
    m_renderer = renderer;
  }

private:
  int m_lastEventPosition[2] = { 0 };
  bool m_leftButtonPress = false;
  vtkImageData* m_contouringImage = nullptr;
  vtkRenderer* m_renderer = nullptr;
};
vtkStandardNewMacro(InteractorStyleImage);


VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_renderer(vtkSmartPointer<vtkRenderer>::New())
    , m_interactorStyle(vtkSmartPointer<InteractorStyleImage>::New())
{
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget()
{

}

void VTKOpenGLWidget::initialize()
{
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
    m_interactorStyle->setRenderer(m_renderer);
    m_renderWindow->GetInteractor()->SetInteractorStyle(m_interactorStyle);
}

void VTKOpenGLWidget::createTestData()
{
  // 3D source sphere
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetPhiResolution(30);
  sphereSource->SetThetaResolution(30);
  sphereSource->SetCenter(0, 0, 0);
  sphereSource->SetRadius(20);

  // generate circle by cutting the sphere with an implicit plane
  // (through its center, axis-aligned)
  vtkNew<vtkCutter> circleCutter;
  circleCutter->SetInputConnection(sphereSource->GetOutputPort());
  vtkNew<vtkPlane> cutPlane;
  cutPlane->SetOrigin(sphereSource->GetCenter());
  cutPlane->SetNormal(0, 0, 1);
  circleCutter->SetCutFunction(cutPlane);

  vtkNew<vtkStripper> stripper;
  stripper->SetInputConnection(circleCutter->GetOutputPort()); // valid circle
  stripper->Update();

  // that's our circle
  auto circle = stripper->GetOutput();

  // prepare the binary image's voxel grid
  vtkNew<vtkImageData> whiteImage;
  double bounds[6];
  circle->GetBounds(bounds);
  double spacing[3]; // desired volume spacing
  spacing[0] = 0.5;
  spacing[1] = 0.5;
  spacing[2] = 0.5;
  whiteImage->SetSpacing(spacing);

  // compute dimensions
  int dim[3];
  for (int i = 0; i < 3; i++)
  {
    dim[i] = static_cast<int>(
                 ceil((bounds[i * 2 + 1] - bounds[i * 2]) / spacing[i])) +
        1;
    if (dim[i] < 1)
      dim[i] = 1;
  }
  whiteImage->SetDimensions(dim);
  whiteImage->SetExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  double origin[3];

  // NOTE: I am not sure whether or not we had to add some offset!
  origin[0] = bounds[0]; // + spacing[0] / 2;
  origin[1] = bounds[2]; // + spacing[1] / 2;
  origin[2] = bounds[4]; // + spacing[2] / 2;
  whiteImage->SetOrigin(origin);
  whiteImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  // fill the image with foreground voxels:
  unsigned char inval = 255;
  unsigned char outval = 0;
  vtkIdType count = whiteImage->GetNumberOfPoints();
  for (vtkIdType i = 0; i < count; ++i)
  {
    whiteImage->GetPointData()->GetScalars()->SetTuple1(i, inval);
  }

  // sweep polygonal data (this is the important thing with contours!)
  vtkNew<vtkLinearExtrusionFilter> extruder;
  extruder->SetInputData(circle);
  extruder->SetScaleFactor(1.);
  extruder->SetExtrusionTypeToVectorExtrusion();
  extruder->SetVector(0, 0, 1);
  extruder->Update();

  // polygonal data --> image stencil:
  vtkNew<vtkPolyDataToImageStencil> pol2stenc;
  pol2stenc->SetTolerance(0); // important if extruder->SetVector(0, 0, 1) !!!
  pol2stenc->SetInputConnection(extruder->GetOutputPort());
  pol2stenc->SetOutputOrigin(origin);
  pol2stenc->SetOutputSpacing(spacing);
  pol2stenc->SetOutputWholeExtent(whiteImage->GetExtent());
  pol2stenc->Update();

  // cut the corresponding white image and set the background:
  vtkNew<vtkImageStencil> imgstenc;
  imgstenc->SetInputData(whiteImage);
  imgstenc->SetStencilConnection(pol2stenc->GetOutputPort());
  imgstenc->ReverseStencilOff();
  imgstenc->SetBackgroundValue(outval);
  imgstenc->Update();
  //imgstenc->GetOutput()->Print(std::cout);


    vtkNew<vtkImageActor> actor;
    actor->SetInputData(imgstenc->GetOutput());

    m_renderer->AddViewProp(actor);
}
