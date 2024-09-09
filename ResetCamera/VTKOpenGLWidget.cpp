#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkTransform.h>
#include <vtkImageActor.h>
#include <vtkNrrdReader.h>
#include <QDebug>
#include <vtkDICOMImageReader.h>
#include <vtkCamera.h>
#include <vtkLinearTransform.h>

class KeyPressInteratorStyle : public vtkInteractorStyleTrackballCamera
{
public:
    static KeyPressInteratorStyle* New();
    vtkTypeMacro(KeyPressInteratorStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnKeyPress() override
    {
        vtkRenderWindowInteractor* rwi = this->Interactor;
        std::string key = rwi->GetKeySym();

        std::cout<<"Pressed "<<key<<endl;

        vtkNew<vtkTransform> transform;
        if (key == "Up")
        {
            AlignImageActor(m_imageActorFirst);
            double firstBounds[6];
            m_imageActorFirst->GetBounds(firstBounds);
            m_renderer->ResetCamera(firstBounds);
        }  
        else if (key == "Down")
        {
            AlignImageActor(m_imageActorSecond);
            double secondBounds[6];
            m_imageActorSecond->GetBounds(secondBounds);
            m_renderer->ResetCamera(secondBounds);
        }  
        else if (key == "Left")
        {
            double firstBounds[6];
            m_imageActorFirst->GetBounds(firstBounds);
            qDebug()<<"First bounds";
            for (int i = 0; i < 6; i++)
            {
                qDebug()<<"firstBounds"<<"["<<i<<"] = "<<firstBounds[i];
            }

            double secondBounds[6];
            m_imageActorSecond->GetBounds(secondBounds);
            qDebug()<<"Second bounds";
            for (int i = 0; i < 6; i++)
            {
                qDebug()<<"secondBounds"<<"["<<i<<"] = "<<secondBounds[i];
            }
        }
        else if (key == "Right")
        {
        }
        m_renderWindow->Render();

        vtkInteractorStyleTrackballCamera::OnKeyPress();
    }

    void SetRenderWindow(vtkRenderWindow* window)
    {
        m_renderWindow = window;
    }

    void SetRenderer(vtkRenderer* renderer)
    {
        m_renderer = renderer;
    }

    void SetImageActorFirst(vtkImageActor* imageActor)
    {
        m_imageActorFirst = imageActor;
    }

    void SetImageActorSecond(vtkImageActor* imageActor)
    {
        m_imageActorSecond = imageActor;
    }

    void AlignImageActor(vtkImageActor* imageActor)
    {
        if (!imageActor)
            return;

        auto userTransform = imageActor->GetUserTransform();
        vtkNew<vtkTransform> transform;
        if (userTransform)
            transform->SetMatrix(userTransform->GetMatrix());

        std::cout<<"transform->GetMatrix() = "<<*transform->GetMatrix();

        double position[3] = {0, 0, -1};
        double viewUp[3] = { 0, 1, 0};
        double focalPoint[3] = { 0, 0, 0};

        auto camera = m_renderer->GetActiveCamera();
        camera->SetPosition(position);
        camera->SetViewUp(viewUp);
        camera->SetFocalPoint(focalPoint);
        camera->ApplyTransform(transform);
    }
private:
    vtkRenderWindow* m_renderWindow;
    vtkRenderer* m_renderer;
    vtkImageActor* m_imageActorFirst;
    vtkImageActor* m_imageActorSecond;
};
vtkStandardNewMacro(KeyPressInteratorStyle);

VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_renderer(vtkSmartPointer<vtkRenderer>::New())
    , m_style(vtkSmartPointer<KeyPressInteratorStyle>::New())
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
    m_renderWindow->GetInteractor()->SetInteractorStyle(m_style);
    m_style->SetRenderWindow(m_renderWindow);
    m_style->SetRenderer(m_renderer);
}

void VTKOpenGLWidget::createTestData()
{
    std::string nrrdFilePath = "D:/MRI.nrrd";
    vtkNew<vtkNrrdReader> nrrdReader;
    nrrdReader->SetFileName(nrrdFilePath.data());
    nrrdReader->Update();

    vtkNew<vtkImageActor> nrrdActor;
    nrrdActor->SetInputData(nrrdReader->GetOutput());
    m_style->SetImageActorFirst(nrrdActor);

    m_renderer->AddActor(nrrdActor);

    std::string directory = "D:/test-data/Patient B/CBL_T2";
    vtkNew<vtkDICOMImageReader> dicomReader;
    dicomReader->SetDirectoryName(directory.data());
    dicomReader->Update();

    vtkNew<vtkTransform> transform;
    transform->Translate(500, 500, 0);
    transform->RotateX(70);
    transform->RotateY(45);

    vtkNew<vtkImageActor> dicomActor;
    dicomActor->SetInputData(dicomReader->GetOutput());
    dicomActor->SetUserTransform(transform);
    m_style->SetImageActorSecond(dicomActor);

    m_renderer->AddActor(dicomActor);
}
