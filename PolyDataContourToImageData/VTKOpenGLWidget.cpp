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
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkCamera.h>
#include <vtkAppendPolyData.h>
#include <vtkGlyph3D.h>
#include <vtkCleanPolyData.h>
#include <vtkPicker.h>
#include <vtkAbstractPicker.h>
#include <vtkAssemblyPath.h>
#include <vtkPropPicker.h>

class InteractorStyleImage : public vtkInteractorStyleImage
{
public:
  static InteractorStyleImage* New();
  vtkTypeMacro(InteractorStyleImage, vtkInteractorStyleImage);

  void OnLeftButtonDown() override
  {
    vtkInteractorStyleImage::OnLeftButtonDown();
    m_leftButtonPress = true;

        int eventPosition[2] = { 0 };
    this->GetInteractor()->GetEventPosition(eventPosition);

      double* worldPosition = nullptr;
      vtkNew<vtkCoordinate> coordinate;
      coordinate->SetCoordinateSystemToDisplay();
      coordinate->SetValue(eventPosition[0], eventPosition[1], 0);
      worldPosition = coordinate->GetComputedWorldValue(m_renderer);
        qDebug()<<"x = "<<worldPosition[0];
        qDebug()<<"y = "<<worldPosition[1];
        qDebug()<<"z = "<<worldPosition[2];


      double newPosition[3] = {
        worldPosition[0],
        worldPosition[1],
        0
      };

    resetLine(newPosition);
  }

  void OnLeftButtonUp() override
  {
    vtkInteractorStyleImage::OnLeftButtonUp();
      m_leftButtonPress = false;

      int x = GetInteractor()->GetEventPosition()[0];
      int y = GetInteractor()->GetEventPosition()[1];

      vtkNew<vtkPropPicker> picker;
      picker->Pick(x, y, 0, m_renderer);
      auto path = picker->GetPath();
      bool validPick = false;
      if (path)
      {
          qDebug()<<"pick number = "<<path->GetNumberOfItems();
          vtkCollectionSimpleIterator sit;
          path->InitTraversal(sit);
          // vtkAssemblyNode *node;
          for (int i = 0; i < path->GetNumberOfItems() && !validPick; ++i)
          {
            auto node = path->GetNextNode(sit);
            vtkImageActor* imageActor = dynamic_cast<vtkImageActor*>(node->GetViewProp());
            if (imageActor)
            {
              validPick = true;
              break;
            }
          }
      }

      qDebug()<<"validPick = "<<validPick;

      if (!validPick)
      {
        return;
      }

      // double pos[3];
      // qDebug()<<"old z = "<<pos[2];
      // picker->GetPickPosition(pos);
      // // Fixes some numerical problems with the picking.
      // //double* bounds = imageActor->GetBounds();
      // int axis = 2;
      // pos[2] = bounds[2 * axis];


      // qDebug()<<"x = "<<pos[0];
      // qDebug()<<"y = "<<pos[1];
      // qDebug()<<"new z = "<<pos[2];

      #if 0

      vtkNew<vtkSphereSource> sphereSource;
      sphereSource->SetPhiResolution(30);
      sphereSource->SetThetaResolution(30);
      sphereSource->SetCenter(0, 0, 0);
      sphereSource->SetRadius(10);
      sphereSource->Update();

      vtkNew<vtkCutter> circleCutter;
      vtkNew<vtkPlane> cutPlane;
      //cutPlane->SetOrigin(point[0], point[1], 0);
      cutPlane->SetOrigin(0, 0, 0);
      cutPlane->SetNormal(0, 0, 1);
      circleCutter->SetCutFunction(cutPlane);
      circleCutter->SetInputConnection(sphereSource->GetOutputPort());

      vtkNew<vtkGlyph3D> glyph3D;
      glyph3D->SetSourceConnection(circleCutter->GetOutputPort());
      glyph3D->SetInputData(m_lineData);

      // vtkNew<vtkAppendPolyData> appendPolydata;
      // appendPolydata->AddInputConnection(glyph3D->GetOutputPort());

      // vtkNew<vtkCleanPolyData> cleanPolydata;
      // cleanPolydata->SetInputConnection(glyph3D->GetOutputPort());


      // vtkNew<vtkLinearExtrusionFilter> extrusionFilter;
      // extrusionFilter->SetInputConnection(glyph3D->GetOutputPort());
      // extrusionFilter->SetScaleFactor(1.);
      // extrusionFilter->SetExtrusionTypeToVectorExtrusion();
      // extrusionFilter->SetVector(0, 0, 1);
      // extrusionFilter->CappingOn();

      double* point = m_linePoints->GetPoint(0);

      vtkNew<vtkPolyDataMapper> mapper;
      mapper->SetInputConnection(glyph3D->GetOutputPort());

      vtkNew<vtkActor> actor;
      actor->SetMapper(mapper);

      m_renderer->AddActor(actor);

      // vtkNew<vtkTransformPolyDataFilter> filter; 

      // vtkNew<vtkAppendPolyData> appendPolyData;
      // for (vtkIdType i = 0; i < m_linePoints->GetNumberOfPoints(); i++)
      // {
      //   vtkNew<vtkTransform> transform;
      //   double* position = m_linePoints->GetPoint(i);
      //   transform->Translate(position[0], position[1], 0);
      //   vtkNew<vtkTransformPolyDataFilter> filter;
      //   filter->SetInputConnection(circleCutter->GetOutputPort());
      //   filter->SetTransform(transform);
      //   filter->Update();

      //   appendPolyData->AddInputConnection(filter->GetOutputPort());
      // }

      // appendPolyData->Update();

      #endif


#if 0
        double origin[3] = { 0 };
  double spacing[3] = { 0.5, 0.5, 0.5 };
  int extent[6] = { 0, 99, 0, 99, 0, 99};

  // polygonal data --> image stencil:
  vtkNew<vtkPolyDataToImageStencil> pol2stenc;
  //pol2stenc->SetTolerance(0); // important if extruder->SetVector(0, 0, 1) !!!
  pol2stenc->SetInputConnection(glyph3D->GetOutputPort());
  //pol2stenc->SetInputData(glyph3D);
  pol2stenc->SetOutputOrigin(origin);
  pol2stenc->SetOutputSpacing(spacing);
  pol2stenc->SetOutputWholeExtent(extent);
  pol2stenc->Update();

  // cut the corresponding white image and set the background:

    // vtkImageData* image = m_imageActor->GetInput();

      vtkNew<vtkImageData> whiteImage;

  whiteImage->SetSpacing(spacing);
  whiteImage->SetExtent(0, 99, 0, 99, 0, 99);
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

    vtkNew<vtkImageStencil> imgstenc;
    imgstenc->SetInputData(whiteImage);
    imgstenc->SetStencilConnection(pol2stenc->GetOutputPort());
    imgstenc->ReverseStencilOff();
    imgstenc->SetBackgroundValue(0);
    imgstenc->Update();

      m_imageActor->SetInputData(imgstenc->GetOutput());

#endif

      m_renderWindow->Render();




  } 

  void OnMouseMove() override
  {
    vtkInteractorStyleImage::OnMouseMove();

    if (!m_leftButtonPress)
      return;



    int eventPosition[2] = { 0 };
    this->GetInteractor()->GetEventPosition(eventPosition);
    double distance = ((m_lastEventPosition[0] - eventPosition[0]) * (m_lastEventPosition[0] - eventPosition[0])) + ((m_lastEventPosition[1] - eventPosition[1]) * (m_lastEventPosition[1] - eventPosition[1]));
    // qDebug()<<"distance = "<<distance;
    if (distance > 0.0)
    {

    // static bool once = false;

    // if (once == false)
    // {
    //     once = true;
    // }
    // else 
    //   return;

      double* worldPosition = nullptr;
      vtkNew<vtkCoordinate> coordinate;
      coordinate->SetCoordinateSystemToDisplay();
      coordinate->SetValue(eventPosition[0], eventPosition[1], 0);
      worldPosition = coordinate->GetComputedWorldValue(m_renderer);
        qDebug()<<"x = "<<worldPosition[0];
        qDebug()<<"y = "<<worldPosition[1];
        qDebug()<<"z = "<<worldPosition[2];


      double newPosition[3] = {
        worldPosition[0],
        worldPosition[1],
        0
      };

      currentPoints[0] = m_pickCount++;
      currentPoints[1] = m_pickCount;

      m_linePoints->InsertPoint(m_pickCount, newPosition);
      m_lineCells->InsertNextCell(2, currentPoints);
      m_lineCells->Modified();

      this->m_linePoints->GetData()->Modified();

      this->m_lineData->SetPoints(this->m_linePoints);
      this->m_lineData->SetLines(this->m_lineCells);
      this->m_lineData->Modified();


      // vtkNew<vtkSphereSource> sphereSource;
      // sphereSource->SetPhiResolution(30);
      // sphereSource->SetThetaResolution(30);
      // sphereSource->SetCenter(0, 0, 0);
      // sphereSource->SetRadius(2);
      // sphereSource->Update();

      // vtkNew<vtkCutter> circleCutter;
      // vtkNew<vtkPlane> cutPlane;
      // cutPlane->SetOrigin(0, 0, 0);
      // cutPlane->SetNormal(0, 0, 1);
      // circleCutter->SetCutFunction(cutPlane);
      // circleCutter->SetInputConnection(sphereSource->GetOutputPort());

      // vtkNew<vtkTransform> transform;
      // transform->Translate(newPosition);
      // vtkNew<vtkTransformPolyDataFilter> filter; 
      // filter->SetTransform(transform);
      // filter->SetInputConnection(circleCutter->GetOutputPort());

      // vtkNew<vtkPolyDataMapper> cutterMapper;
      // cutterMapper->SetInputConnection(filter->GetOutputPort());

      // vtkNew<vtkActor> cutterActor;
      // cutterActor->SetMapper(cutterMapper);

      // this->m_renderer->AddActor(cutterActor);

      this->m_renderWindow->Render();


  // 3D source sphere
  // vtkNew<vtkSphereSource> sphereSource;
  // sphereSource->SetPhiResolution(30);
  // sphereSource->SetThetaResolution(30);
  // sphereSource->SetCenter(0, 0, 0);
  // sphereSource->SetRadius(2);

  // // generate circle by cutting the sphere with an implicit plane
  // // (through its center, axis-aligned)

  // vtkNew<vtkCutter> circleCutter;
  // vtkNew<vtkPlane> cutPlane;
  // cutPlane->SetOrigin(0, 0, 0);
  // cutPlane->SetNormal(0, 0, 1);
  // circleCutter->SetCutFunction(cutPlane);
  // circleCutter->SetInputConnection(sphereSource->GetOutputPort());

  // vtkNew<vtkStripper> stripper;
  // stripper->SetInputConnection(circleCutter->GetOutputPort()); // valid circle
  // stripper->Update();

  // // that's our circle
  // auto circle = stripper->GetOutput();

  // // sweep polygonal data (this is the important thing with contours!)
  // vtkNew<vtkLinearExtrusionFilter> extruder;
  // //extruder->SetInputConnection(stripper->GetOutputPort());
  // extruder->SetInputData(circle);
  // extruder->SetScaleFactor(1.);
  // extruder->SetExtrusionTypeToVectorExtrusion();
  // extruder->SetVector(0, 0, 1);
  // extruder->Update();

  // vtkNew<vtkTransform> transform;
  // transform->Translate(worldPosition[0], worldPosition[1], 0);

  // vtkNew<vtkTransformFilter> transformFilter;
  // transformFilter->SetInputConnection(extruder->GetOutputPort());
  // transformFilter->SetTransform(transform);


  // vtkNew<vtkPolyDataMapper> cutterMapper;
  // cutterMapper->SetInputConnection(transformFilter->GetOutputPort());
  // //cutterMapper->SetResolveCoincidentTopologyToPolygonOffset();  

  // vtkNew<vtkNamedColors> colors;
  // vtkNew<vtkActor> cutterActor;
  // cutterActor->GetProperty()->SetColor(colors->GetColor3d("red").GetData());
  // cutterActor->SetMapper(cutterMapper);
  // m_renderer->AddActor(cutterActor);


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

  void setBaseImage(vtkImageData* image)
  {
    m_baseImage = image;
  }

  void setLinePoints(vtkPoints* linePoints)
  {
    m_linePoints = linePoints;
  }

  void setLineCells(vtkCellArray* lineCells)
  {
    m_lineCells = lineCells;
  }

  void setLineData(vtkPolyData* lineData)
  {
    m_lineData = lineData;
  }

  void resetLine(double* pos)
  {
    this->m_pickCount = 0;
    this->m_linePoints->InsertPoint(m_pickCount, pos);
  }

private:
  int m_lastEventPosition[2] = { 0 };
  bool m_leftButtonPress = false;
  vtkImageData* m_contouringImage = nullptr;
  vtkRenderer* m_renderer = nullptr;
  vtkRenderWindow* m_renderWindow;
  vtkImageActor* m_imageActor;
  vtkImageData* m_baseImage;
  vtkPoints* m_linePoints;
  vtkCellArray* m_lineCells;
  vtkPolyData* m_lineData;
  int m_pickCount = 0;

  vtkIdType currentPoints[2];
};
vtkStandardNewMacro(InteractorStyleImage);


VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_renderer(vtkSmartPointer<vtkRenderer>::New())
    , m_interactorStyle(vtkSmartPointer<InteractorStyleImage>::New())
    , m_linePoints(vtkSmartPointer<vtkPoints>::New())
    , m_lineCells(vtkSmartPointer<vtkCellArray>::New())
    , m_lineData(vtkSmartPointer<vtkPolyData>::New())
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
  initColor(whiteImage, 255);


  vtkNew<vtkImageData> grayImage;
  grayImage->DeepCopy(whiteImage);
  initColor(grayImage, 150);

    vtkNew<vtkImageActor> actor;
    actor->SetInputData(grayImage);

    m_interactorStyle->setBaseImage(grayImage);
    m_interactorStyle->setImageActor(actor);
    m_interactorStyle->setLinePoints(m_linePoints);
    m_interactorStyle->setLineCells(m_lineCells);
    m_interactorStyle->setLineData(m_lineData);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(m_lineData);

    vtkNew<vtkActor> lineActor;
    lineActor->SetMapper(mapper);
    lineActor->GetProperty()->SetColor(0, 0, 0);
    lineActor->GetProperty()->SetLineWidth(2);
    lineActor->GetProperty()->SetRepresentationToWireframe();
    lineActor->GetProperty()->SetInterpolationToFlat();

    m_renderer->AddActor(lineActor);
    m_renderer->AddViewProp(actor);
}

void VTKOpenGLWidget::initColor(vtkImageData *image, const int &color)
{
    vtkIdType count = image->GetNumberOfPoints();
    for (vtkIdType i = 0; i < count; ++i)
    {
      image->GetPointData()->GetScalars()->SetTuple1(i, color);
    }
}
