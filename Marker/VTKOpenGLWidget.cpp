#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkSphere.h>
#include <vtkVectorText.h>
#include <vtkPolyDataMapper.h>
#include <vtkFollower.h>
#include <vtkSphereSource.h>
#include <vtkCylinderSource.h>
#include <vtkTransformFilter.h>
#include <vtkTransform.h>
#include <vtkAssembly.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkProp3DFollower.h>
#include <vtkTransformPolyDataFilter.h>
#include "MarkerAssembly.h"
#include <QPushButton>
#include <QDebug>
#include <vtkProperty2D.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>


class KeyPressInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
    static KeyPressInteractorStyle* New();
    vtkTypeMacro(KeyPressInteractorStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnKeyPress() override
    {
        vtkRenderWindowInteractor* rwi = this->Interactor;
        std::string key = rwi->GetKeySym();

        std::cout<<"Pressed "<<key<<endl;

        vtkNew<vtkTransform> transform;
        vtkLinearTransform* userTransform = mProp3D->GetUserTransform();
        if (key == "Up")
        {
            transform->SetInput(userTransform);
            transform->Translate(0, 1, 0);
            mProp3D->SetUserTransform(transform);
        }  
        else if (key == "Down")
        {
            transform->SetInput(userTransform);
            transform->Translate(0, -1, 0);
            mProp3D->SetUserTransform(transform);
        }  
        else if (key == "Left")
        {
            transform->SetInput(userTransform);
            transform->Translate(-1, 0, 0);
            mProp3D->SetUserTransform(transform);
        }
        else if (key == "Right")
        {
            transform->SetInput(userTransform);
            transform->Translate(1, 0, 0);
            mProp3D->SetUserTransform(transform);
        }

        double *bounds = mProp3D->GetBounds();
        for (int i = 0; i < 6; ++i)
        {
            qDebug()<<i<<" = "<<bounds[i];
        }
        mRenderWindow->Render();

        vtkInteractorStyleTrackballCamera::OnKeyPress();
    }
    void SetActor(vtkActor *actor)
    {
        mActor = actor;
    }

    void SetProp3D(vtkProp3D *prop3D)
    {
        mProp3D = prop3D;
    }

    void SetRenderWindow(vtkGenericOpenGLRenderWindow* window)
    {
        mRenderWindow = window;
    }
private:
    vtkActor* mActor = nullptr;
    vtkProp3D* mProp3D = nullptr;
    vtkGenericOpenGLRenderWindow* mRenderWindow = nullptr;
};
vtkStandardNewMacro(KeyPressInteractorStyle);

VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_renderer(vtkSmartPointer<vtkRenderer>::New())
    , m_style(vtkSmartPointer<KeyPressInteractorStyle>::New())
    , m_markerAssembly(vtkSmartPointer<MarkerAssembly>::New())
    , m_captionActor(vtkSmartPointer<vtkCaptionActor2D>::New())
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

    auto it = m_renderWindow->GetInteractor();
    m_style->SetRenderWindow(m_renderWindow);
    //it->SetInteractorStyle(m_style);


    // QPushButton* btn = new QPushButton(this);
    // connect(btn, &QPushButton::clicked, this, [this]{
    //     m_markerAssembly->setText("CCCCC");
    //     m_markerAssembly->setColor(1.0, 0, 0);
    //     m_renderWindow->Render();
    // });
}

void VTKOpenGLWidget::createTestData()
{
    vtkNew<vtkCylinderSource> cylinder;
    cylinder->SetRadius(5);
    cylinder->SetHeight(20);
    cylinder->SetResolution(100);

    vtkNew<vtkTransform> cylinderTransform;
    cylinderTransform->RotateX(90);

    vtkNew<vtkTransformPolyDataFilter> cylinderTransformFilter;
    cylinderTransformFilter->SetInputConnection(cylinder->GetOutputPort());
    cylinderTransformFilter->SetTransform(cylinderTransform);

    vtkNew<vtkPolyDataMapper> cylinderMapper;
    cylinderMapper->SetInputConnection(cylinderTransformFilter->GetOutputPort());

    vtkNew<vtkActor> cylinderActor;
    cylinderActor->SetMapper(cylinderMapper);

    m_renderer->AddActor(cylinderActor);


    vtkNew<vtkTransform> textTransform;
    textTransform->Translate(0, 10, 0);

    double point[3] = { 0 };
    double newPoint[3] = { 0 };
    textTransform->TransformPoint(point, newPoint);

    m_captionActor->SetCaption("T1");
    m_captionActor->ThreeDimensionalLeaderOff();
    m_captionActor->LeaderOff();
    m_captionActor->BorderOff();
    m_captionActor->SetPosition(0, 0);
    m_captionActor->SetAttachmentPoint(newPoint);
    //m_captionActor->GetCaptionTextProperty()->SetColor(1.0, 0, 0);

    m_renderer->AddActor(m_captionActor);
}
