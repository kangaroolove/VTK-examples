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
#include <itkImage.h>
#include <itkImageSeriesReader.h>
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImageToVTKImageFilter.h>
#include <vtkMatrix4x4.h>
#include <vtkImageReslice.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkCamera.h>
#include <vtkImageActor.h>
#include <vtkAxesActor.h>

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
            // double origin[3];
            // mImageActor->GetOrigin(origin);
            // for (int i = 0; i < 3; ++i)
            // {
            //     qDebug()<<"origin["<<i<<"] = "<<origin[i];
            // }
            // double bounds[6];
            // mImageActor->GetBounds(bounds);
            // for (int i = 0; i < 6; i++)
            // {
            //     qDebug()<<"bounds["<<i<<"] = "<<bounds[i];
            // }
        }  
        else if (key == "Left")
        {
            auto camera = mRenderer->GetActiveCamera();
            double position[3];
            camera->GetPosition(position);
            qDebug()<<"Camera position";
            for (int i = 0; i < 3; ++i)
                qDebug()<<position[i];

            double focalPoint[3];
            camera->GetFocalPoint(focalPoint);
            qDebug()<<"Camera focalPoint";
            for (int i = 0; i < 3; ++i)
                qDebug()<<focalPoint[i];
        }
        else if (key == "Right")
        {

        }

        vtkInteractorStyleTrackballCamera::OnKeyPress();
    }

    void SetImageSlice(vtkImageSlice *slice)
    {
        mImageSlice = slice;
    }

    void SetImageActor(vtkImageActor* actor)
    {
        mImageActor = actor;
    }

    void SetRenderWindow(vtkRenderWindow* window)
    {
        mRenderWindow = window;
    }

    void SetRenderer(vtkRenderer* renderer)
    {
        mRenderer = renderer;
    }
private:
    vtkImageActor* mImageActor;
    vtkImageSlice* mImageSlice;
    vtkRenderWindow* mRenderWindow;
    vtkRenderer* mRenderer;
};
vtkStandardNewMacro(KeyPressInteractorStyle);

VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_renderer(vtkSmartPointer<vtkRenderer>::New())
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
    m_renderer->SetBackground(0.0, 1.0, 0.0);
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
    m_renderWindow->GetInteractor()->SetInteractorStyle(m_style);
    m_style->SetRenderer(m_renderer);
}

void VTKOpenGLWidget::createTestData()
{
    std::string dir = "D:/Standard test-data/Set N - Problematic dicom/CHENKAI/20230324080608/702";
    int index = 0;
    double level = 1245;

    using ImageType = itk::Image<short, 3>;
    using ReaderType = itk::ImageSeriesReader<ImageType>;

    auto itkReader = ReaderType::New();
    using ImageIOType = itk::GDCMImageIO;
    auto dicomIO = ImageIOType::New();
    
    itkReader->SetImageIO(dicomIO);

    using NamesGeneratorType = itk::GDCMSeriesFileNames;
    auto nameGenerator = NamesGeneratorType::New();

    nameGenerator->SetUseSeriesDetails(true);
    nameGenerator->AddSeriesRestriction("0008|0021");
    nameGenerator->SetDirectory(dir);

    using SeriesIdContainer = std::vector<std::string>;
    const SeriesIdContainer & seriesUID = nameGenerator->GetSeriesUIDs();
    std::string seriesIdentifier;
    seriesIdentifier = seriesUID.begin()->c_str();

    using FileNamesContainer = std::vector<std::string>;
    FileNamesContainer fileNames;
    fileNames = nameGenerator->GetFileNames(seriesIdentifier);
    itkReader->SetFileNames(fileNames);
    try 
    {
        itkReader->Update();
    }
    catch (...)
    {

    }

    auto direction = itkReader->GetOutput()->GetDirection();
    
    // Get direction matrix
    vtkNew<vtkMatrix4x4> matrix;
    matrix->Identity();

    for (unsigned int i = 0; i < 3; ++i)
    {
        for (unsigned int j = 0; j < 3; ++j)
        {
            matrix->SetElement(i, j, direction(i, j));
        }
    }

    using FilterType = itk::ImageToVTKImageFilter<ImageType>;
    FilterType::Pointer filter = FilterType::New();
    filter->SetInput(itkReader->GetOutput());
    filter->Update();

    // Get the VTK image
    auto image = filter->GetOutput();
    if (image)
    {
        qDebug()<<"converted successfully";
        //image->Print(std::cout);
    }

    vtkNew<vtkImageReslice> reslice;
    reslice->SetInputData(filter->GetOutput());
    reslice->Update();

    reslice->GetOutput()->Print(std::cout);



    vtkNew<vtkTransform> imageActorTransform;
    imageActorTransform->SetMatrix(matrix);
    imageActorTransform->Translate(20, 0, 0);

    vtkNew<vtkImageActor> imageActor;
    imageActor->SetInputData(reslice->GetOutput());
    imageActor->GetProperty()->SetColorWindow(level * 2);
    imageActor->GetProperty()->SetColorLevel(level);
    imageActor->SetUserTransform(imageActorTransform);
    //imageActor->SetUserMatrix(matrix);
    m_renderer->AddViewProp(imageActor);
    m_style->SetImageActor(imageActor);

    vtkNew<vtkAxesActor> axesActor;
    m_renderer->AddActor(axesActor);

    double position[3] = {0, 0, 1};
    double viewUp[3] = { 0, 1, 0};
    double focalPoint[3] = { 0, 0, 0};

    auto camera = m_renderer->GetActiveCamera();
    camera->SetPosition(position);
    camera->SetViewUp(viewUp);
    camera->SetFocalPoint(focalPoint);

    // vtkNew<vtkTransform> transform;
    // transform->Concatenate(matrix);
    //camera->ApplyTransform(imageActorTransform);
    m_renderer->ResetCamera();
#if 0
    // If I don't use this, the application will crash
    vtkNew<vtkImageReslice> reslice;
    reslice->SetInputData(filter->GetOutput());
    reslice->Update();

    reslice->GetOutput()->Print(std::cout);

    vtkNew<vtkTransform> transform;
    transform->SetMatrix(matrix);

    vtkNew<vtkImageResliceMapper> mapper;
    mapper->SetInputData(reslice->GetOutput());

    // qDebug()<<"mapper->GetSliceAtFocalPoint() = "<<mapper->GetSliceAtFocalPoint();
    // qDebug()<<"mapper->SetSliceFacesCamera() = "<<mapper->GetSliceFacesCamera();

    mapper->SetSliceAtFocalPoint(true);
    mapper->SetSliceFacesCamera(true);

    vtkNew<vtkImageSlice> slice;
    slice->SetMapper(mapper);

    slice->GetProperty()->SetColorWindow(level * 2);
    slice->GetProperty()->SetColorLevel(level);
    slice->SetUserMatrix(matrix);

    m_renderer->AddViewProp(slice);
    m_style->SetImageSlice(slice);

    m_renderer->GetActiveCamera()->ApplyTransform(transform);
    m_renderer->ResetCamera();
    #endif
}
