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

    void aa() {
        int a = 0;
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
    it->SetInteractorStyle(m_style);


    QPushButton* btn = new QPushButton(this);
    connect(btn, &QPushButton::clicked, this, [this]{
        m_markerAssembly->setText("CCCCC");
        m_renderWindow->Render();
    });
}

void VTKOpenGLWidget::createTestData()
{
    #if 0
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



    vtkNew<vtkVectorText> text;
    text->SetText("ABC");

    vtkNew<vtkTransform> textTransform;
    textTransform->Translate(-2, 10, 0);

    vtkNew<vtkTransformPolyDataFilter> textTransformFilter;
    textTransformFilter->SetInputConnection(text->GetOutputPort());
    textTransformFilter->SetTransform(textTransform);

    vtkNew<vtkPolyDataMapper> textMapper;
    textMapper->SetInputConnection(textTransformFilter->GetOutputPort());

    vtkNew<vtkActor> textActor;
    textActor->SetMapper(textMapper);

    vtkNew<vtkAssembly> assembly;
    assembly->AddPart(textActor);
    assembly->AddPart(cylinderActor);

    m_renderer->AddActor(assembly);
    m_style->SetProp3D(assembly);

    m_renderer->ResetCamera();
    #endif

    m_renderer->AddActor(m_markerAssembly);
}
