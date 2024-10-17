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
#include <vtkImageProperty.h>

class KeyPressInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
    static KeyPressInteractorStyle *New();
    vtkTypeMacro(KeyPressInteractorStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnKeyPress() override
    {
        vtkRenderWindowInteractor *rwi = this->Interactor;
        std::string key = rwi->GetKeySym();

        std::cout << "Pressed " << key << endl;
        if (key == "Up")
        {
            double bounds[6];
            mImageActor->GetBounds(bounds);
            for (int i = 0; i < 6; i++)
            {
                qDebug() << "bounds[" << i << "] = " << bounds[i];
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
            qDebug() << "Camera position";
            for (int i = 0; i < 3; ++i)
                qDebug() << position[i];

            double focalPoint[3];
            camera->GetFocalPoint(focalPoint);
            qDebug() << "Camera focalPoint";
            for (int i = 0; i < 3; ++i)
                qDebug() << focalPoint[i];
        }
        else if (key == "Right")
        {
        }

        vtkInteractorStyleTrackballCamera::OnKeyPress();
    }

    void OnMouseWheelForward()
    {
        qDebug() << "OnMouseWheelForward";
        auto camera = mRenderer->GetActiveCamera();
        auto focalPoint = camera->GetFocalPoint();
        camera->SetFocalPoint(focalPoint[0], focalPoint[1], focalPoint[2] + 3);
        mRenderWindow->Render();
        // vtkInteractorStyleTrackballCamera::OnMouseWheelForward();
    }

    void OnMouseWheelBackward()
    {
        qDebug() << "OnMouseWheelBackward";
        auto camera = mRenderer->GetActiveCamera();
        auto focalPoint = camera->GetFocalPoint();
        camera->SetFocalPoint(focalPoint[0], focalPoint[1], focalPoint[2] - 3);
        mRenderWindow->Render();
        // vtkInteractorStyleTrackballCamera::OnMouseWheelBackward();
    }

    void SetImageSlice(vtkImageSlice *slice)
    {
        mImageSlice = slice;
    }

    void SetImageActor(vtkImageActor *actor)
    {
        mImageActor = actor;
    }

    void SetRenderWindow(vtkRenderWindow *window)
    {
        mRenderWindow = window;
    }

    void SetRenderer(vtkRenderer *renderer)
    {
        mRenderer = renderer;
    }

private:
    vtkImageActor *mImageActor;
    vtkImageSlice *mImageSlice;
    vtkRenderWindow *mRenderWindow;
    vtkRenderer *mRenderer;
};
vtkStandardNewMacro(KeyPressInteractorStyle);

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent), m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()), m_renderer(vtkSmartPointer<vtkRenderer>::New()), m_style(vtkSmartPointer<KeyPressInteractorStyle>::New())
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
    m_style->SetRenderWindow(m_renderWindow);
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
    const SeriesIdContainer &seriesUID = nameGenerator->GetSeriesUIDs();
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
    vtkNew<vtkMatrix4x4> directionMatrix;
    directionMatrix->Identity();

    for (unsigned int i = 0; i < 3; ++i)
    {
        for (unsigned int j = 0; j < 3; ++j)
        {
            directionMatrix->SetElement(i, j, direction(i, j));
        }
    }

    using FilterType = itk::ImageToVTKImageFilter<ImageType>;
    FilterType::Pointer filter = FilterType::New();
    filter->SetInput(itkReader->GetOutput());
    filter->Update();

    vtkNew<vtkImageReslice> reslice;
    reslice->SetInputData(filter->GetOutput());
    reslice->Update();
    // reslice->GetOutput()->Print(std::cout);
#if 0
    vtkNew<vtkImageActor> imageActor;
    imageActor->SetInputData(reslice->GetOutput());
    imageActor->GetProperty()->SetColorWindow(level * 2);
    imageActor->GetProperty()->SetColorLevel(level);
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
#else
    // If I don't use this, the application will crash
    vtkNew<vtkTransform> transform;
    transform->SetMatrix(directionMatrix);

    std::cout << "directionMatrix = " << *directionMatrix << endl;

    double verticalNormal[3] = {
        directionMatrix->GetElement(0, 1),
        directionMatrix->GetElement(1, 1),
        directionMatrix->GetElement(2, 1),
    };

    double horizontalNormal[3] = {
        directionMatrix->GetElement(0, 0),
        directionMatrix->GetElement(1, 0),
        directionMatrix->GetElement(2, 0),
    };

    double planeNormal[3] = {
        directionMatrix->GetElement(0, 2),
        directionMatrix->GetElement(1, 2),
        directionMatrix->GetElement(2, 2),
    };

    double newCameraPosition[3];
    double tempPosition[3] = {0};

    auto image = reslice->GetOutput();
    int *dimension = image->GetDimensions();

    for (int i = 0; i < 3; ++i)
    {
        std::cout << "dimension[" << i << "] = " << dimension[i] << endl;
    }

    int imageIJK[3] = {
        dimension[0] / 2,
        dimension[1] / 2,
        dimension[2] / 2,
    };
    imageIJK[2] = 0;
    double calculateFocalPoint[3] = {0};
    calculateWorldPositionFromImageIJK(image, directionMatrix, imageIJK, calculateFocalPoint);
    printArray("calculateFocalPoint", calculateFocalPoint);

    int newimageIJK[3] = {
        imageIJK[0],
        imageIJK[1],
        imageIJK[2] + 100,
    };
    double otherPosition[3] = {0};
    calculateWorldPositionFromImageIJK(image, directionMatrix, newimageIJK, otherPosition);

    double spacing[3];
    image->GetSpacing(spacing);

    double origin[3];
    image->GetOrigin(origin);

    vtkNew<vtkImageResliceMapper> mapper;
    mapper->SetInputData(reslice->GetOutput());
    mapper->SetSliceFacesCamera(true);
    mapper->SetSliceAtFocalPoint(true);

    vtkNew<vtkImageSlice> slice;
    slice->SetMapper(mapper);

    vtkNew<vtkTransform> originTransform;
    originTransform->PostMultiply();
    originTransform->Translate(-origin[0], -origin[1], -origin[2]);
    originTransform->Concatenate(directionMatrix);
    originTransform->Translate(origin[0], origin[1], origin[2]);

    slice->GetProperty()->SetColorWindow(level * 2);
    slice->GetProperty()->SetColorLevel(level);
    slice->GetProperty()->SetInterpolationTypeToNearest();
    slice->SetUserTransform(originTransform);

    m_renderer->AddViewProp(slice);
    m_style->SetImageSlice(slice);
    double sliceBounds[6];
    slice->GetBounds(sliceBounds);
    for (int i = 0; i < 6; i++)
        std::cout << sliceBounds[i] << ",";
    std::cout << std::endl;

    double viewUpVector[3];
    viewUpVector[0] = -verticalNormal[0];
    viewUpVector[1] = -verticalNormal[1];
    viewUpVector[2] = -verticalNormal[2];

    printArray("calculateViewUp", viewUpVector);

    double destPositionVector[3];
    double length = 300;
    destPositionVector[0] = -planeNormal[0] * length;
    destPositionVector[1] = -planeNormal[1] * length;
    destPositionVector[2] = -planeNormal[2] * length;

    double bounds[6] = {0};
    image->GetBounds(bounds);

    vtkMath::Add(calculateFocalPoint, destPositionVector, newCameraPosition);

    // m_renderer->ResetCamera();
    m_renderer->GetActiveCamera()->SetFocalPoint(calculateFocalPoint);
    m_renderer->GetActiveCamera()->SetViewUp(viewUpVector);
    m_renderer->GetActiveCamera()->SetPosition(newCameraPosition);

    // m_renderer->ResetCameraClippingRange();
    double clippingRange[2];
    // m_renderer->GetActiveCamera()->GetClippingRange(clippingRange);
    // std::cout<<"clippingRange "<<clippingRange[0]<<", "<<clippingRange[1]<<endl;

    // double normal[3];
    // vtkMath::Subtract(calculateFocalPoint, newCameraPosition, normal);
    // vtkMath::Normalize(normal);
    // printArray("normal", normal);
    return;
    m_renderer->ResetCamera();
    std::cout << "reset===========================================" << endl;

    double correctfocalPoint[3] = {0};
    double cameraViewUp[3] = {0};
    double cameraPosition[3] = {0};
    m_renderer->GetActiveCamera()->GetFocalPoint(correctfocalPoint);
    m_renderer->GetActiveCamera()->GetViewUp(cameraViewUp);
    m_renderer->GetActiveCamera()->GetPosition(cameraPosition);
    // m_renderer->GetActiveCamera()->GetClippingRange(clippingRange);
    // std::cout<<"clippingRange "<<clippingRange[0]<<", "<<clippingRange[1]<<endl;

    printArray("cameraViewUp", cameraViewUp);
    printArray("correctFocalPoint", correctfocalPoint);
    printArray("correctPositionPoint", cameraPosition);

    double focalPointVector[3];
    vtkMath::Subtract(calculateFocalPoint, correctfocalPoint, focalPointVector);
    // vtkMath::Normalize(focalPointVector);
    printArray("FocalPointDifferenceVector", focalPointVector);

    double correctCameraPositionVector[3];
    vtkMath::Subtract(correctfocalPoint, cameraPosition, correctCameraPositionVector);
    vtkMath::Normalize(correctCameraPositionVector);
    printArray("correctCameraPositionVector", correctCameraPositionVector);
#endif
}

void VTKOpenGLWidget::calculateWorldPositionFromImageIJK(vtkImageData *image, vtkMatrix4x4 *directionMatrix, int in[3], double out[3])
{
    if (!image)
        return;

    double imageIJK[] = {in[0], in[1], in[2]};
    double spacing[3];
    image->GetSpacing(spacing);

    double spacingVector[] = {
        imageIJK[0] * spacing[0],
        imageIJK[1] * spacing[1],
        imageIJK[2] * spacing[2],
    };

    double origin[3];
    image->GetOrigin(origin);

    double vector[3];
    vtkNew<vtkTransform> transform;
    transform->SetMatrix(directionMatrix);
    transform->TransformPoint(spacingVector, vector);

    for (int i = 0; i < 3; ++i)
        out[i] = vector[i] + origin[i];
}

void VTKOpenGLWidget::calculateImageIJKFromWorldPosition(vtkImageData *image, vtkMatrix4x4 *directionMatrix, double in[3], int out[3])
{
    if (!image)
        return;

    double origin[3];
    image->GetOrigin(origin);

    double spacing[3];
    image->GetSpacing(spacing);

    double point[3];
    point[0] = in[0] - origin[0];
    point[1] = in[1] - origin[1];
    point[2] = in[2] - origin[2];

    double vector[3];
    vtkNew<vtkMatrix4x4> matrix;
    matrix->DeepCopy(directionMatrix);
    matrix->Invert();
    matrix->MultiplyPoint(point, vector);

    vector[0] /= spacing[0];
    vector[1] /= spacing[1];
    vector[2] /= spacing[2];

    out[0] = std::round(vector[0]);
    out[1] = std::round(vector[1]);
    out[2] = std::round(vector[2]);
}

void VTKOpenGLWidget::printArray(const std::string &name, double array[3])
{
    ;
    std::cout << name << " " << array[0] << ", " << array[1] << ", " << array[2] << endl;
}
