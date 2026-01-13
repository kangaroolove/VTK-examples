#include "VTKOpenGLWidget.h"
#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkAngleWidget.h>
#include <vtkAngleRepresentation3D.h>
#include <vtkAngleRepresentation2D.h>
#include <vtkMath.h>
#include <QDebug>

class vtkAngleCallback : public vtkCommand
{
public:
  vtkTypeMacro(vtkAngleCallback, vtkCommand);

  static vtkAngleCallback* New()
  {
    return new vtkAngleCallback;
  }

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long eid,
               void* vtkNotUsed(callData)) override
  {
    if (eid == vtkCommand::PlacePointEvent)
    {
      std::cout << "point placed\n";
    }
    else if (eid == vtkCommand::InteractionEvent)
    {
      double point1[3], center[3], point2[3];
      this->Rep->GetPoint1WorldPosition(point1);
      this->Rep->GetCenterWorldPosition(center);
      this->Rep->GetPoint2WorldPosition(point2);
      std::cout << "Angle between "
                << "(" << point1[0] << "," << point1[1] << "," << point1[2] << "), (" << center[0]
                << "," << center[1] << "," << center[2] << ") and (" << point2[0] << ","
                << point2[1] << "," << point2[2] << ") is " << this->Rep->GetAngle() << " radians."
                << ", Angle = " <<vtkMath::DegreesFromRadians(Rep->GetAngle())
                << std::endl;
    }
    else if (eid == vtkCommand::EndInteractionEvent)
    {
        std::cout<<"Angle is ="<< this->Rep->GetAngle() << " radians."
                << ", Angle = " <<vtkMath::DegreesFromRadians(Rep->GetAngle())<<endl
                << "Use 3.14 = "<<Rep->GetAngle() * 180.0 / 3.14
                <<endl;
    }

  }
  
    vtkAngleRepresentation3D* Rep;
  vtkAngleCallback()
    : Rep(nullptr)
  {
  }

};

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_renderer(vtkSmartPointer<vtkRenderer>::New()),
      m_angleWidget(vtkSmartPointer<vtkAngleWidget>::New()),
      m_angleCallBack(vtkSmartPointer<vtkAngleCallback>::New()) {
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData() {
    vtkNew<vtkAngleRepresentation3D> rep;
    rep->SetLabelFormat("%-#6.2f");

    m_angleWidget->SetInteractor(m_renderWindow->GetInteractor());
    m_angleWidget->CreateDefaultRepresentation();

    m_angleWidget->SetRepresentation(rep);
    m_angleCallBack->Rep = rep;
    m_angleWidget->AddObserver(vtkCommand::EndInteractionEvent,
                               m_angleCallBack);
    m_angleWidget->On();
}
