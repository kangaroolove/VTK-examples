#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkImageActor.h>
#include <vtkNrrdReader.h>
#include <vtkInteractorStyleImage.h>
#include <vtkObjectFactory.h>
#include <vtkCamera.h>

class AStyle : public vtkInteractorStyleImage
{
public:
    static AStyle* New();
    vtkTypeMacro(AStyle, vtkInteractorStyleImage);

    virtual void OnKeyPress() override
    {
        vtkRenderWindowInteractor* rwi = this->Interactor;
        std::string key = rwi->GetKeySym();

        std::cout<<"Pressed "<<key<<endl;

        // vtkNew<vtkTransform> transform;
        // vtkLinearTransform* userTransform = mConeActor->GetUserTransform();
        if (key == "Up")
        {
             int displayExtent[6] = { 0 };
            mImageActor->GetDisplayExtent(displayExtent);
            for (int i = 0; i < 6; i++)
            {
                std::cout<<"Extent["<<i<<"] = "<<displayExtent[i]<<endl;
            }

            mImageActor->SetDisplayExtent(5, 5, 0, 511, 0, 19);

            mCamera->SetFocalPoint(0,0,0);
            mCamera->SetPosition(1,0,0); // -1 if medical ?
            mCamera->SetViewUp(0,0,1);
            //std::cout<<mCamera->GetParallelScale()<<endl;
            mRenderer->ResetCamera();


            std::cout<<"Slice number ="<<mImageActor->GetSliceNumber()<<endl;

            std::cout<<"z slice number = "<<mImageActor->GetZSlice();
            // transform->SetInput(userTransform);
            // transform->Translate(0, 1, 0);
            // mConeActor->SetUserTransform(transform);
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
    // void SetConeActor(vtkActor *coneActor)
    // {
    //     mConeActor = coneActor;
    // }

    void SetImageActor(vtkImageActor* actor)
    {
        mImageActor = actor;
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
    vtkImageActor* mImageActor;
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

void VTKOpenGLWidget::initialize()
{
    m_renderer->SetBackground(1.0, 1.0, 0.0);
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::initInteraction()
{
    m_style->SetRenderWindow(m_renderWindow);
    m_style->SetRenderer(m_renderer);
    m_style->SetInteractionModeToImage3D();
    m_renderWindow->GetInteractor()->SetInteractorStyle(m_style);
    m_style->SetCamera(m_renderer->GetActiveCamera());
    m_renderer->ResetCamera();
}

void VTKOpenGLWidget::createTestData()
{
   std::vector<std::string> fileNames = {
        "D:/MRI.nrrd",
        "D:/CBJ_T2.nrrd"
   };

    for (unsigned int i = 0; i < 1; i++)
    {
        vtkNew<vtkNrrdReader> reader;
        reader->SetFileName(fileNames[i].data());
        reader->Update();

        int displayExtent[6] = { 0 };
        reader->GetDataExtent(displayExtent);
        for (int i = 0; i < 6; i++)
        {
         std::cout<<"Extent["<<i<<"] = "<<displayExtent[i]<<endl;
        }

        vtkNew<vtkImageActor> actor;
        actor->SetInputData(reader->GetOutput());

        std::cout<<"max = "<<actor->GetSliceNumberMax()<<endl;
        std::cout<<"min = "<<actor->GetSliceNumberMin()<<endl;

        m_renderer->AddViewProp(actor);
        m_style->SetImageActor(actor);
    }
}
