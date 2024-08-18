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
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNamedColors.h>
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkTransformPolyDataFilter.h>

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
    //vtkInteractorStyleImage::OnMouseMove();
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
        qDebug()<<"x = "<<worldPosition[0];
        qDebug()<<"y = "<<worldPosition[1];
        qDebug()<<"z = "<<worldPosition[2];


  // 3D source sphere
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetPhiResolution(30);
  sphereSource->SetThetaResolution(30);
  sphereSource->SetCenter(0, 0, 0);
  sphereSource->SetRadius(20);

  // generate circle by cutting the sphere with an implicit plane
  // (through its center, axis-aligned)

  vtkNew<vtkCutter> circleCutter;
  vtkNew<vtkPlane> cutPlane;
  cutPlane->SetOrigin(0, 0, 0);
  cutPlane->SetNormal(0, 0, 1);
  circleCutter->SetCutFunction(cutPlane);
  circleCutter->SetInputConnection(sphereSource->GetOutputPort());

  vtkNew<vtkStripper> stripper;
  stripper->SetInputConnection(circleCutter->GetOutputPort()); // valid circle
  stripper->Update();

  // that's our circle
  auto circle = stripper->GetOutput();

  // sweep polygonal data (this is the important thing with contours!)
  vtkNew<vtkLinearExtrusionFilter> extruder;
  //extruder->SetInputConnection(stripper->GetOutputPort());
  extruder->SetInputData(circle);
  extruder->SetScaleFactor(1.);
  extruder->SetExtrusionTypeToVectorExtrusion();
  extruder->SetVector(0, 0, 1);
  extruder->Update();

  vtkNew<vtkTransform> transform;
  transform->Translate(worldPosition);

  vtkNew<vtkTransformFilter> transformFilter;
  transformFilter->SetInputConnection(extruder->GetOutputPort());
  transformFilter->SetTransform(transform);


  // vtkNew<vtkPolyDataMapper> cutterMapper;
  // cutterMapper->SetInputConnection(transformFilter->GetOutputPort());
  // //cutterMapper->SetResolveCoincidentTopologyToPolygonOffset();  

  // vtkNew<vtkNamedColors> colors;
  // vtkNew<vtkActor> cutterActor;
  // cutterActor->GetProperty()->SetColor(colors->GetColor3d("red").GetData());
  // cutterActor->SetMapper(cutterMapper);
  // m_renderer->AddActor(cutterActor);

    #if 1

  double origin[3] = { 0 };
  double spacing[3] = { 0.5 };
  int extent[6] = { 0, 99, 0, 99, 0, 99};

  // polygonal data --> image stencil:
  vtkNew<vtkPolyDataToImageStencil> pol2stenc;
  pol2stenc->SetTolerance(0); // important if extruder->SetVector(0, 0, 1) !!!
  pol2stenc->SetInputConnection(transformFilter->GetOutputPort());
  //pol2stenc->SetInputData(extruder->GetOutput());
  pol2stenc->SetOutputOrigin(origin);
  pol2stenc->SetOutputSpacing(spacing);
  pol2stenc->SetOutputWholeExtent(extent);
  pol2stenc->Update();

  // cut the corresponding white image and set the background:

    vtkImageData* image = m_imageActor->GetInput();

    vtkNew<vtkImageStencil> imgstenc;
    imgstenc->SetInputData(image);
    imgstenc->SetStencilConnection(pol2stenc->GetOutputPort());
    //imgstenc->SetStencilData(pol2stenc->GetOutput());
    imgstenc->ReverseStencilOff();
    imgstenc->SetBackgroundValue(255.0);
    imgstenc->Update();

      m_imageActor->SetInputData(imgstenc->GetOutput());
      #endif

      m_renderWindow->Render();
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

  void setRenderWindow(vtkRenderWindow* renderWindow)
  {
    m_renderWindow = renderWindow;
  }

  void setImageActor(vtkImageActor* vtkImageActor)
  {
    m_imageActor = vtkImageActor;
  }

private:
  int m_lastEventPosition[2] = { 0 };
  bool m_leftButtonPress = false;
  vtkImageData* m_contouringImage = nullptr;
  vtkRenderer* m_renderer = nullptr;
  vtkRenderWindow* m_renderWindow;
  vtkImageActor* m_imageActor;
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
    m_renderer->SetBackground(1.0, 1.0, 0.0);
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
    m_interactorStyle->setRenderer(m_renderer);
    m_interactorStyle->setRenderWindow(m_renderWindow);
    m_renderWindow->GetInteractor()->SetInteractorStyle(m_interactorStyle);
}

void VTKOpenGLWidget::createTestData()
{
  // prepare the binary image's voxel grid
  vtkNew<vtkImageData> whiteImage;
  double spacing[3]; // desired volume spacing
  spacing[0] = 0.5;
  spacing[1] = 0.5;
  spacing[2] = 0.5;
  whiteImage->SetSpacing(spacing);
  whiteImage->SetExtent(0, 99, 0, 99, 0, 99);
  double origin[3] = { 0 };
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

    vtkNew<vtkImageActor> actor;
    actor->SetInputData(whiteImage);

      m_interactorStyle->setImageActor(actor);

    m_renderer->AddViewProp(actor);
}
