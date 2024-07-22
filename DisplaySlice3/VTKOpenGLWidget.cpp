#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkImageResliceMapper.h>
#include <vtkNrrdReader.h>
#include <vtkImageSlice.h>
#include <vtkInteractorStyleImage.h>
#include <vtkCamera.h>
#include <vtkImageActor.h>
#include <vtkInteractorStyleTrackballCamera.h>

class AStyle : public vtkInteractorStyleTrackballCamera
{
public:
    static AStyle* New();
    vtkTypeMacro(AStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnKeyPress() override
    {
        vtkRenderWindowInteractor* rwi = this->Interactor;
        std::string key = rwi->GetKeySym();

        std::cout<<"Pressed "<<key<<endl;

        // vtkNew<vtkTransform> transform;
        // vtkLinearTransform* userTransform = mConeActor->GetUserTransform();
        if (key == "Up")
        {
            double focalPoint[3] = { 0 };
            mCamera->GetFocalPoint(focalPoint);

            std::cout<<"camera focal point"<<endl;
            for (int i = 0; i < 3; i++)
                std::cout<<focalPoint[i]<<endl;


            std::cout<<"camera position"<<endl;
            double position[3] = { 0 };
            mCamera->GetPosition(position);
            for (int i = 0; i < 3; i++)
                std::cout<<position[i]<<endl;

            std::cout<<"camera viewUp"<<endl;
            double viewUp[3] = { 0 };
            mCamera->GetViewUp(viewUp);
            for (int i = 0; i < 3; i++)
                std::cout<<viewUp[i]<<endl;
        }  
        else if (key == "Down")
        {
            // transform->SetInput(userTransform);
            // transform->Translate(0, -1, 0);
            // mConeActor->SetUserTransform(transform);
        }  
        else if (key == "Left")
        {
            // transform->SetInput(userTransform);
            // transform->Translate(-1, 0, 0);
            // mConeActor->SetUserTransform(transform);
        }
        else if (key == "Right")
        {
            // transform->SetInput(userTransform);
            // transform->Translate(1, 0, 0);
            // mConeActor->SetUserTransform(transform);
        }
        mRenderWindow->Render();

        vtkInteractorStyleTrackballCamera::OnKeyPress();
    }

    void SetRenderWindow(vtkRenderWindow* window)
    {
        mRenderWindow = window;
    }

    void SetCamera(vtkCamera* camera)
    {
        mCamera = camera;
    }

    void SetRenderer(vtkRenderer* renderer)
    {
        mRenderer = renderer;
    }
private:
    vtkRenderWindow* mRenderWindow;
    vtkCamera* mCamera;
    vtkRenderer* mRenderer;
};
vtkStandardNewMacro(AStyle);

VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_renderer(vtkSmartPointer<vtkRenderer>::New())
    , m_style(vtkSmartPointer<AStyle>::New())
{
    initialize();
    createTestData();
    initInteraction();
}

VTKOpenGLWidget::~VTKOpenGLWidget()
{

}

void VTKOpenGLWidget::initInteraction()
{
    m_style->SetRenderWindow(m_renderWindow);
    m_style->SetRenderer(m_renderer);
    m_renderWindow->GetInteractor()->SetInteractorStyle(m_style);
    m_style->SetCamera(m_renderer->GetActiveCamera());
    m_renderer->ResetCamera();
}

void VTKOpenGLWidget::initialize()
{
    m_renderer->SetBackground(1.0, 1.0, 0.0);
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData()
{
    std::vector<std::string> fileNames = {
        "D:/3D-USScan_20210316144218.nrrd",
        "D:/CBJ_T2.nrrd"
    };

    for (auto& fileName : fileNames)
    {
        vtkNew<vtkNrrdReader> reader;
        reader->SetFileName(fileName.data());
        reader->Update();

        // vtkNew<vtkImageResliceMapper> mapper;
        // mapper->SetInputConnection(reader->GetOutputPort());

        // vtkNew<vtkImageSlice> slice;
        // slice->SetMapper(mapper);

        // m_renderer->AddViewProp(slice);

        // std::cout<<fileName<<endl;
        // double bounds[6] = { 0 };
        // std::cout<<"bounds"<<endl;
        // slice->GetBounds(bounds);
        // for (int i = 0; i < 6; i++)
        // {
        //     std::cout<<bounds[i]<<endl;
        // }


        vtkNew<vtkImageActor> actor;
        actor->SetInputData(reader->GetOutput());
        std::cout<<fileName<<endl;
        double bounds[6] = { 0 };
        std::cout<<"bounds"<<endl;
        actor->GetBounds(bounds);
        for (int i = 0; i < 6; i++)
        {
            std::cout<<bounds[i]<<endl;
        }

        int extent[6];
        reader->GetDataExtent(extent);

        std::cout<<"Extent"<<endl;
        for (int i = 0; i < 6; i++)
            std::cout<<extent[i]<<endl;

        actor->SetDisplayExtent(extent[0], extent[1], extent[2], extent[3], 0, 0);
        m_renderer->AddViewProp(actor);
    }
}
