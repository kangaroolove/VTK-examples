#include "VTKOpenGLWidget.h"
#include "MarkerPointActor.h"
#include <QDebug>
#include <QPushButton>
#include <vtkActor.h>
#include <vtkAssembly.h>
#include <vtkConeSource.h>
#include <vtkCylinderSource.h>
#include <vtkFollower.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProp3DFollower.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphere.h>
#include <vtkSphereSource.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVectorText.h>

class KeyPressInteractorStyle : public vtkInteractorStyleTrackballCamera {
public:
  static KeyPressInteractorStyle *New();
  vtkTypeMacro(KeyPressInteractorStyle, vtkInteractorStyleTrackballCamera);

  virtual void OnKeyPress() override {
    vtkRenderWindowInteractor *rwi = this->Interactor;
    std::string key = rwi->GetKeySym();

    std::cout << "Pressed " << key << endl;

    vtkNew<vtkTransform> transform;
    vtkLinearTransform *userTransform = mProp->GetUserTransform();
    if (key == "Up") {
      transform->SetInput(userTransform);
      transform->Translate(0, 1, 0);
      mProp->SetUserMatrix(transform->GetMatrix());
    } else if (key == "Down") {
      transform->SetInput(userTransform);
      transform->Translate(0, -1, 0);
      mProp->SetUserMatrix(transform->GetMatrix());
    } else if (key == "Left") {
      transform->SetInput(userTransform);
      transform->Translate(-1, 0, 0);
      mProp->SetUserMatrix(transform->GetMatrix());
    } else if (key == "Right") {
      transform->SetInput(userTransform);
      transform->Translate(1, 0, 0);
      mProp->SetUserMatrix(transform->GetMatrix());
    }

    double *bounds = mProp->GetBounds();
    for (int i = 0; i < 6; ++i) {
      qDebug() << i << " = " << bounds[i];
    }
    mRenderWindow->Render();

    vtkInteractorStyleTrackballCamera::OnKeyPress();
  }
  void SetActor(vtkActor *actor) { mActor = actor; }

  void setProp3D(vtkProp3D *prop) { mProp = prop; }

  void SetRenderWindow(vtkGenericOpenGLRenderWindow *window) {
    mRenderWindow = window;
  }

private:
  vtkActor *mActor = nullptr;
  vtkProp3D *mProp = nullptr;
  vtkGenericOpenGLRenderWindow *mRenderWindow = nullptr;
};
vtkStandardNewMacro(KeyPressInteractorStyle);

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_renderer(vtkSmartPointer<vtkRenderer>::New()),
      m_style(vtkSmartPointer<KeyPressInteractorStyle>::New()),
      m_captionActor(vtkSmartPointer<vtkCaptionActor2D>::New()),
      m_transform(vtkSmartPointer<vtkTransform>::New()),
      m_markerActor(vtkSmartPointer<MarkerPointActor>::New()) {
  initialize();
  createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
  m_renderWindow->AddRenderer(m_renderer);
  SetRenderWindow(m_renderWindow);

  auto it = m_renderWindow->GetInteractor();
  m_style->SetRenderWindow(m_renderWindow);
  it->SetInteractorStyle(m_style);
}

void VTKOpenGLWidget::createTestData() {
  m_markerActor->setText("KangarooLove");
  m_renderer->AddActor(m_markerActor);
  m_style->setProp3D(m_markerActor);
}
