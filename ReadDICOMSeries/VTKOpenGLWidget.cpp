#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkDICOMImageReader.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>
#include <vtkImageProperty.h>
#include <QDebug>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkImageSlice.h>
#include <vtkImageResliceMapper.h>
#include <vtkPlane.h>
#include <vtkImageData.h>

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
        if (key == "Up")
        {
            double bounds[6];
            mImageActor->GetBounds(bounds);
            for (int i = 0; i < 6; i++)
            {
                qDebug()<<"bounds["<<i<<"] = "<<bounds[i];
            }
        }  
        else if (key == "Down")
        {
            double bounds[6];
            mImageSlice->GetBounds(bounds);
            for (int i = 0; i < 6; i++)
            {
                qDebug()<<"bounds["<<i<<"] = "<<bounds[i];
            }
        }  
        else if (key == "Left")
        {
        }
        else if (key == "Right")
        {

        }

        vtkInteractorStyleTrackballCamera::OnKeyPress();
    }
    void SetImageActor(vtkImageActor *actor)
    {
        mImageActor = actor;
    }

    void SetImageSlice(vtkImageSlice *slice)
    {
        mImageSlice = slice;
    }

    void SetRenderWindow(vtkRenderWindow* window)
    {
        mRenderWindow = window;
    }
private:
    vtkImageActor* mImageActor;
    vtkImageSlice* mImageSlice;
    vtkRenderWindow* mRenderWindow;
};
vtkStandardNewMacro(KeyPressInteractorStyle);

VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_rendererLeft(vtkSmartPointer<vtkRenderer>::New())
    , m_rendererRight(vtkSmartPointer<vtkRenderer>::New())
    , m_style(vtkSmartPointer<KeyPressInteractorStyle>::New())
{
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget()
{

}

void VTKOpenGLWidget::initialize()
{
    m_rendererLeft->SetViewport(0, 0, 0.5, 1.0);
    m_rendererLeft->SetBackground(1.0, 0.0, 0.0);

    m_rendererRight->SetViewport(0.5, 0, 1.0, 1.0);
    m_rendererRight->SetBackground(0.0, 1.0, 0.0);

    m_renderWindow->AddRenderer(m_rendererLeft);
    m_renderWindow->AddRenderer(m_rendererRight);
    SetRenderWindow(m_renderWindow);
    m_renderWindow->GetInteractor()->SetInteractorStyle(m_style);
}

void VTKOpenGLWidget::createTestData()
{
    std::string dir = "D:/Standard test-data/Set B - realPatient/Patient B/CBL_T2";
    //std::string dir = "D:/Standard test-data/Set B - realPatient/Patient A - Registration/t2";
    vtkNew<vtkDICOMImageReader> reader;
    reader->SetDirectoryName(dir.data());
    reader->Update();

    double origin[3];
    reader->GetDataOrigin(origin);
    qDebug()<<"data origin[0] = "<<origin[0]<<", data origin[1] = "<<origin[1]<<", data origin[2] = "<<origin[2];

    int extent[6];
    reader->GetDataExtent(extent);

    reader->GetOutput()->Print(std::cout);

    int index = 0;
    double level = 883;

    vtkNew<vtkImageActor> actor;
    actor->SetInputData(reader->GetOutput());
    actor->GetProperty()->SetColorWindow(level * 2);
    actor->GetProperty()->SetColorLevel(level);
    actor->SetDisplayExtent(extent[0], extent[1], extent[2], extent[3], index, index);
    m_rendererLeft->AddViewProp(actor);

    vtkNew<vtkImageResliceMapper> mapper;
    mapper->SetInputConnection(reader->GetOutputPort());

    vtkNew<vtkImageSlice> slice;
    slice->SetMapper(mapper);
    slice->GetProperty()->SetColorWindow(level * 2);
    slice->GetProperty()->SetColorLevel(level);
    m_rendererRight->AddViewProp(slice);

    m_style->SetImageActor(actor);
    m_style->SetImageSlice(slice);
}
