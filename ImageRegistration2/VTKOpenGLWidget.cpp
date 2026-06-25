#include "VTKOpenGLWidget.h"

#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImage.h>
#include <itkImageSeriesReader.h>
#include <itkImageToVTKImageFilter.h>
#include <itkMetaDataObject.h>
#include <itkNrrdImageIO.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkConeSource.h>
#include <vtkDICOMImageReader.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkImageProperty.h>
#include <vtkImageReslice.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLookupTable.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

#include <QDebug>

class KeyPressInteractorStyle : public vtkInteractorStyleTrackballCamera {
public:
    static KeyPressInteractorStyle *New();
    vtkTypeMacro(KeyPressInteractorStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnKeyPress() override {
        vtkRenderWindowInteractor *rwi = this->Interactor;
        std::string key = rwi->GetKeySym();

        std::cout << "Pressed " << key << endl;
        if (key == "Up") {
            // double bounds[6];
            // mImageActor->GetBounds(bounds);
            // for (int i = 0; i < 6; i++)
            // {
            //     qDebug()<<"bounds["<<i<<"] = "<<bounds[i];
            // }
        } else if (key == "Down") {
            double bounds[6];
            mImageSlice->GetBounds(bounds);
            for (int i = 0; i < 6; i++) {
                qDebug() << "bounds[" << i << "] = " << bounds[i];
            }
        } else if (key == "Left") {
            auto camera = mRenderer->GetActiveCamera();
            double position[3];
            camera->GetPosition(position);
            qDebug() << "Camera position";
            for (int i = 0; i < 3; ++i) qDebug() << position[i];

            double focalPoint[3];
            camera->GetFocalPoint(focalPoint);
            qDebug() << "Camera focalPoint";
            for (int i = 0; i < 3; ++i) qDebug() << focalPoint[i];
        } else if (key == "Right") {
        }

        vtkInteractorStyleTrackballCamera::OnKeyPress();
    }

    void SetImageSlice(vtkImageSlice *slice) { mImageSlice = slice; }

    void SetRenderWindow(vtkRenderWindow *window) { mRenderWindow = window; }

    void SetRenderer(vtkRenderer *renderer) { mRenderer = renderer; }

private:
    vtkImageActor *mImageActor;
    vtkImageSlice *mImageSlice;
    vtkRenderWindow *mRenderWindow;
    vtkRenderer *mRenderer;
};
vtkStandardNewMacro(KeyPressInteractorStyle);

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_renderer(vtkSmartPointer<vtkRenderer>::New()),
      m_style(vtkSmartPointer<KeyPressInteractorStyle>::New()) {
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
    m_renderer->SetBackground(0.0, 1.0, 0.0);
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
    m_renderWindow->GetInteractor()->SetInteractorStyle(m_style);
    m_style->SetRenderer(m_renderer);
}

vtkSmartPointer<vtkImageData> VTKOpenGLWidget::loadDICOMImage(const std::string &dir,
                                                               vtkSmartPointer<vtkMatrix4x4> &outMatrix) {
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

    for (auto series : seriesUID) {
        auto fileNames = nameGenerator->GetFileNames(series);
        for (auto fileName : fileNames) std::cout << fileName << std::endl;
    }

    std::string seriesIdentifier = seriesUID.begin()->c_str();
    itkReader->SetFileNames(nameGenerator->GetFileNames(seriesIdentifier));
    try {
        itkReader->Update();
    } catch (...) {
    }

    using DictionaryType = itk::MetaDataDictionary;
    const DictionaryType &dictionary = dicomIO->GetMetaDataDictionary();

    using MetaDataStringType = itk::MetaDataObject<std::string>;
    auto it = dictionary.Find("0020|0011");
    if (it != dictionary.End()) {
        MetaDataStringType::Pointer entryvalue =
            dynamic_cast<MetaDataStringType *>(it->second.GetPointer());
        if (entryvalue)
            std::cout << it->first << " = " << entryvalue->GetMetaDataObjectValue() << std::endl;
    }

    auto direction = itkReader->GetOutput()->GetDirection();
    outMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    outMatrix->Identity();
    for (unsigned int i = 0; i < 3; ++i)
        for (unsigned int j = 0; j < 3; ++j)
            outMatrix->SetElement(i, j, direction(i, j));

    using FilterType = itk::ImageToVTKImageFilter<ImageType>;
    FilterType::Pointer filter = FilterType::New();
    filter->SetInput(itkReader->GetOutput());
    filter->Update();

    if (filter->GetOutput())
        qDebug() << "converted successfully";

    // If I don't use this, the application will crash
    vtkNew<vtkImageReslice> reslice;
    reslice->SetInputData(filter->GetOutput());
    reslice->Update();
    reslice->GetOutput()->Print(std::cout);

    vtkSmartPointer<vtkImageData> result = reslice->GetOutput();
    return result;
}

vtkSmartPointer<vtkImageData> VTKOpenGLWidget::loadNrrdImage(
    const std::string &dir, vtkSmartPointer<vtkMatrix4x4> &outMatrix) {
    using ImageType = itk::Image<short, 3>;
    using ReaderType = itk::ImageFileReader<ImageType>;

    auto itkReader = ReaderType::New();
    auto nrrdIO = itk::NrrdImageIO::New();
    itkReader->SetImageIO(nrrdIO);
    itkReader->SetFileName(dir);
    try {
        itkReader->Update();
    } catch (...) {
    }

    auto direction = itkReader->GetOutput()->GetDirection();
    outMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    outMatrix->Identity();
    for (unsigned int i = 0; i < 3; ++i)
        for (unsigned int j = 0; j < 3; ++j)
            outMatrix->SetElement(i, j, direction(i, j));

    using FilterType = itk::ImageToVTKImageFilter<ImageType>;
    FilterType::Pointer filter = FilterType::New();
    filter->SetInput(itkReader->GetOutput());
    filter->Update();

    if (filter->GetOutput()) qDebug() << "converted successfully";

    // If I don't use this, the application will crash
    vtkNew<vtkImageReslice> reslice;
    reslice->SetInputData(filter->GetOutput());
    reslice->Update();
    reslice->GetOutput()->Print(std::cout);

    vtkSmartPointer<vtkImageData> result = reslice->GetOutput();
    return result;
}

void VTKOpenGLWidget::createTestData() {
    std::string MriDir =
        "D:/Standard test-data-V3/Set B - Real Patient/Patient A - "
        "Registration/t2";

    vtkSmartPointer<vtkMatrix4x4> MRIMatrix;
    auto MriImageData = loadDICOMImage(MriDir, MRIMatrix);

    vtkNew<vtkImageResliceMapper> MriMapper;
    MriMapper->SetInputData(MriImageData);

    vtkNew<vtkPlane> plane;
    plane->SetOrigin(0, 0, 0);
    plane->SetNormal(0, 0, 1);

    vtkNew<vtkLookupTable> MriLut;
    MriLut->SetRange(MriImageData->GetScalarRange());
    MriLut->SetHueRange(0.0, 0.0);
    MriLut->SetSaturationRange(0.0, 0.0);
    MriLut->SetValueRange(0.0, 1.0);
    MriLut->SetRampToLinear();
    MriLut->Build();

    vtkNew<vtkImageSlice> MriSlice;
    MriSlice->SetMapper(MriMapper);

    MriSlice->GetProperty()->UseLookupTableScalarRangeOn();
    MriSlice->GetProperty()->SetLookupTable(MriLut);
    MriSlice->SetUserMatrix(MRIMatrix);

    m_renderer->AddViewProp(MriSlice);
    // m_style->SetImageSlice(MriSlice);

    std::string ultrasoundDir =
        "D:/Standard test-data-V3/Set B - Real Patient/Patient A - "
        "Registration/3D-USScan_20201029101446 - 000 NEW.nrrd";

    vtkSmartPointer<vtkMatrix4x4> usMatrix;
    auto usData = loadNrrdImage(ultrasoundDir, usMatrix);

    vtkNew<vtkImageResliceMapper> usMapper;
    usMapper->SetInputData(usData);

    vtkNew<vtkLookupTable> usLut;
    usLut->SetRange(usData->GetScalarRange());
    usLut->SetHueRange(0.0, 0.0);
    usLut->SetSaturationRange(0.0, 0.0);
    usLut->SetValueRange(0.0, 1.0);
    usLut->SetRampToLinear();
    usLut->Build();

    vtkNew<vtkImageSlice> usSlice;
    usSlice->SetMapper(usMapper);

    usSlice->GetProperty()->UseLookupTableScalarRangeOn();
    usSlice->GetProperty()->SetLookupTable(usLut);
    usSlice->SetUserMatrix(usMatrix);

    m_renderer->AddViewProp(usSlice);

    m_renderer->GetActiveCamera()->Pitch(180);
    m_renderer->GetActiveCamera()->Roll(180);
    // m_renderer->GetActiveCamera()->ApplyTransform(transform);

    m_renderer->ResetCamera();
}
