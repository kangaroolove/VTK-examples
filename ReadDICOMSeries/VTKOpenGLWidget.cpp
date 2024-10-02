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
            // double bounds[6];
            // mImageActor->GetBounds(bounds);
            // for (int i = 0; i < 6; i++)
            // {
            //     qDebug()<<"bounds["<<i<<"] = "<<bounds[i];
            // }
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
    //m_rendererLeft->SetViewport(0, 0, 0.5, 1.0);
    //m_rendererLeft->SetBackground(1.0, 0.0, 0.0);

    //m_rendererRight->SetViewport(0.5, 0, 1.0, 1.0);
    m_rendererRight->SetBackground(0.0, 1.0, 0.0);

    //m_renderWindow->AddRenderer(m_rendererLeft);
    m_renderWindow->AddRenderer(m_rendererRight);
    SetRenderWindow(m_renderWindow);
    m_renderWindow->GetInteractor()->SetInteractorStyle(m_style);
    m_style->SetRenderer(m_rendererRight);
}

void VTKOpenGLWidget::createTestData()
{
    std::string dir = "D:/Standard test-data/Set B - realPatient/Patient B/CBL_T2";
    int index = 0;
    double level = 883;

    //std::string dir = "D:/Standard test-data/Set B - realPatient/Patient A - Registration/t2";
    // vtkNew<vtkDICOMImageReader> reader;
    // reader->SetDirectoryName(dir.data());
    // reader->Update();

    // double origin[3];
    // reader->GetDataOrigin(origin);
    // qDebug()<<"data origin[0] = "<<origin[0]<<", data origin[1] = "<<origin[1]<<", data origin[2] = "<<origin[2];

    // int extent[6];
    // reader->GetDataExtent(extent);

    // reader->GetOutput()->Print(std::cout);

    // vtkNew<vtkImageActor> actor;
    // actor->SetInputData(reader->GetOutput());
    // actor->GetProperty()->SetColorWindow(level * 2);
    // actor->GetProperty()->SetColorLevel(level);
    // actor->SetDisplayExtent(extent[0], extent[1], extent[2], extent[3], index, index);
    // m_rendererLeft->AddViewProp(actor);
    // m_style->SetImageActor(actor);

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

    qDebug()<<"itk image";
    //itkReader->GetOutput()->Print(std::cout);
    qDebug()<<"--------------------------------------------------------------";

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

    vtkNew<vtkTransform> transform;
    transform->SetMatrix(matrix);

    // vtkNew<vtkTransformFilter> transformFilter;
    // transformFilter->SetInputConnection(reslice->GetOutputPort());
    // transformFilter->SetTransform(transform);

    vtkNew<vtkImageResliceMapper> mapper;
    mapper->SetInputData(reslice->GetOutput());

    qDebug()<<"mapper->GetSliceAtFocalPoint() = "<<mapper->GetSliceAtFocalPoint();
    qDebug()<<"mapper->SetSliceFacesCamera() = "<<mapper->GetSliceFacesCamera();

    mapper->SetSliceAtFocalPoint(true);
    mapper->SetSliceFacesCamera(true);

    vtkNew<vtkImageSlice> slice;
    slice->SetMapper(mapper);

    slice->GetProperty()->SetColorWindow(level * 2);
    slice->GetProperty()->SetColorLevel(level);
    slice->SetUserMatrix(matrix);
    m_rendererRight->AddViewProp(slice);
    m_style->SetImageSlice(slice);

    m_rendererRight->GetActiveCamera()->ApplyTransform(transform);
    m_rendererRight->ResetCamera();
}
