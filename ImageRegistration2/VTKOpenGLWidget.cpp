#include "VTKOpenGLWidget.h"

#include <itkCastImageFilter.h>
#include <itkCenteredTransformInitializer.h>
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageRegistrationMethodv4.h>
#include <itkImageSeriesReader.h>
#include <itkImageToVTKImageFilter.h>
#include <itkMattesMutualInformationImageToImageMetricv4.h>
#include <itkMetaDataObject.h>
#include <itkNrrdImageIO.h>
#include <itkRegistrationParameterScalesFromPhysicalShift.h>
#include <itkRegularStepGradientDescentOptimizerv4.h>
#include <itkTransformFileWriter.h>
#include <itkVersorRigid3DTransform.h>
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

using RegImageType = itk::Image<float, 3>;
static constexpr unsigned int kRegistrationWorkUnits = 4;
static constexpr unsigned int kRegistrationSeed = 42;

bool runRigidRegistration(RegImageType *fixed, RegImageType *moving,
                          const double *fixedCenter, const double *movingCenter,
                          vtkMatrix4x4 *fixedToMoving, std::string &errorMessage);

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

vtkSmartPointer<vtkImageData> VTKOpenGLWidget::loadDICOMImage(
    const std::string &dir, vtkSmartPointer<vtkMatrix4x4> &outMatrix,
    itk::Image<short, 3>::Pointer &outItkImage) {
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

    outItkImage = itkReader->GetOutput();

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
    const std::string &dir, vtkSmartPointer<vtkMatrix4x4> &outMatrix,
    itk::Image<short, 3>::Pointer &outItkImage) {
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

    outItkImage = itkReader->GetOutput();

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
    auto MriImageData = loadDICOMImage(MriDir, MRIMatrix, m_mriItkImage);

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

    m_mriSlice = vtkSmartPointer<vtkImageSlice>::New();
    m_mriSlice->SetMapper(MriMapper);
    m_mriSlice->GetProperty()->UseLookupTableScalarRangeOn();
    m_mriSlice->GetProperty()->SetLookupTable(MriLut);
    m_mriSlice->SetUserMatrix(MRIMatrix);

    m_renderer->AddViewProp(m_mriSlice);

    std::string ultrasoundDir =
        "D:/Standard test-data-V3/Set B - Real Patient/Patient A - "
        "Registration/3D-USScan_20201029101446 - 000 NEW.nrrd";

    vtkSmartPointer<vtkMatrix4x4> usMatrix;
    auto usData = loadNrrdImage(ultrasoundDir, usMatrix, m_usItkImage);

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

    m_renderer->ResetCamera();
}

void VTKOpenGLWidget::runRegistration() {
    if (!m_usItkImage || !m_mriItkImage || !m_mriSlice) {
        qDebug() << "Images not loaded, cannot run registration";
        return;
    }

    // Cast short→float; mutual-information works better on float images
    using CastFilter = itk::CastImageFilter<itk::Image<short, 3>, RegImageType>;

    auto castFixed = CastFilter::New();
    castFixed->SetInput(m_usItkImage);
    castFixed->Update();

    auto castMoving = CastFilter::New();
    castMoving->SetInput(m_mriItkImage);
    castMoving->Update();

    vtkNew<vtkMatrix4x4> fixedToMoving;
    std::string errorMessage;
    bool ok = runRigidRegistration(castFixed->GetOutput(), castMoving->GetOutput(),
                                    nullptr, nullptr, fixedToMoving, errorMessage);
    if (!ok) {
        qDebug() << "Registration failed:" << QString::fromStdString(errorMessage);
        return;
    }

    // fixedToMoving maps US (fixed) → MRI (moving) physical space.
    // Invert to get MRI → US, then pre-multiply the MRI actor's current
    // direction transform so it renders in the US/world frame.
    vtkNew<vtkMatrix4x4> movingToFixed;
    vtkMatrix4x4::Invert(fixedToMoving, movingToFixed);

    vtkNew<vtkMatrix4x4> newMriMatrix;
    vtkMatrix4x4::Multiply4x4(movingToFixed, m_mriSlice->GetUserMatrix(), newMriMatrix);
    m_mriSlice->SetUserMatrix(newMriMatrix);

    m_renderWindow->Render();
}

// Rigidly registers moving onto fixed using Mattes mutual information, which is
// suited to multi-modal pairs such as ultrasound (fixed) and MRI (moving).
// When fixedCenter and movingCenter are both non-null (the prostate landmarks,
// in each image's physical coordinates) the initial transform is the
// translation that maps the fixed landmark onto the moving one, with the
// rotation centered on the fixed landmark; otherwise the volumes' geometric
// centers are aligned. On success, fixedToMoving holds the optimized rigid
// transform as a 4x4 homogeneous matrix mapping fixed-image physical points to
// moving-image physical points (both in LPS world coordinates).
bool runRigidRegistration(RegImageType *fixed, RegImageType *moving,
                          const double *fixedCenter, const double *movingCenter,
                          vtkMatrix4x4 *fixedToMoving,
                          std::string &errorMessage) {
    typedef itk::VersorRigid3DTransform<double> TransformType;
    typedef itk::RegularStepGradientDescentOptimizerv4<double> OptimizerType;
    typedef itk::MattesMutualInformationImageToImageMetricv4<RegImageType,
                                                             RegImageType>
        MetricType;
    typedef itk::ImageRegistrationMethodv4<RegImageType, RegImageType,
                                           TransformType>
        RegistrationType;

    MetricType::Pointer metric = MetricType::New();
    OptimizerType::Pointer optimizer = OptimizerType::New();
    RegistrationType::Pointer registration = RegistrationType::New();
    registration->SetMetric(metric);
    registration->SetOptimizer(optimizer);

    metric->SetNumberOfHistogramBins(32);
    // Fix the work-unit count so the threaded reduction is order-deterministic.
    metric->SetMaximumNumberOfWorkUnits(kRegistrationWorkUnits);

    // Build the initial transform (identity rotation in both cases).
    TransformType::Pointer initialTransform = TransformType::New();
    if (fixedCenter && movingCenter) {
        // Landmark initialization: rotate about the fixed prostate center and
        // translate it onto the moving prostate center. With identity rotation
        // the transform then maps fixedCenter exactly onto movingCenter, so the
        // optimizer starts already aligned at the prostate.
        TransformType::InputPointType center;
        TransformType::OutputVectorType translation;
        for (int i = 0; i < 3; ++i) {
            center[i] = fixedCenter[i];
            translation[i] = movingCenter[i] - fixedCenter[i];
        }
        initialTransform->SetIdentity();
        initialTransform->SetCenter(center);
        initialTransform->SetTranslation(translation);
    } else {
        // Fall back to aligning the geometric centers of the two volumes.
        typedef itk::CenteredTransformInitializer<TransformType, RegImageType,
                                                  RegImageType>
            InitializerType;
        InitializerType::Pointer initializer = InitializerType::New();
        initializer->SetTransform(initialTransform);
        initializer->SetFixedImage(fixed);
        initializer->SetMovingImage(moving);
        initializer->GeometryOn();
        initializer->InitializeTransform();
    }
    registration->SetInitialTransform(initialTransform);

    // Scale the rotation and translation parameters consistently so a single
    // step length is meaningful for both.
    typedef itk::RegistrationParameterScalesFromPhysicalShift<MetricType>
        ScalesEstimatorType;
    ScalesEstimatorType::Pointer scalesEstimator = ScalesEstimatorType::New();
    scalesEstimator->SetMetric(metric);
    scalesEstimator->SetTransformForward(true);
    optimizer->SetScalesEstimator(scalesEstimator);

    optimizer->SetLearningRate(0.5);
    optimizer->SetMinimumStepLength(0.001);
    optimizer->SetNumberOfIterations(50);
    optimizer->SetRelaxationFactor(0.6);
    optimizer->SetReturnBestParametersAndValue(true);
    registration->SetNumberOfWorkUnits(kRegistrationWorkUnits);

    // Random sampling seeded for reproducibility. 10% gives a stable MI
    // gradient so the optimizer needs fewer iterations from the pre-aligned
    // starting point.
    registration->SetMetricSamplingStrategy(RegistrationType::RANDOM);
    registration->SetMetricSamplingPercentage(0.10);
    registration->MetricSamplingReinitializeSeed(kRegistrationSeed);

    // Coarse-to-fine for a wider capture range.
    const unsigned int numberOfLevels = 3;
    RegistrationType::ShrinkFactorsArrayType shrinkFactors;
    shrinkFactors.SetSize(numberOfLevels);
    shrinkFactors[0] = 4;
    shrinkFactors[1] = 2;
    shrinkFactors[2] = 1;
    RegistrationType::SmoothingSigmasArrayType smoothingSigmas;
    smoothingSigmas.SetSize(numberOfLevels);
    smoothingSigmas[0] = 2.0;
    smoothingSigmas[1] = 1.0;
    smoothingSigmas[2] = 0.0;
    registration->SetNumberOfLevels(numberOfLevels);
    registration->SetShrinkFactorsPerLevel(shrinkFactors);
    registration->SetSmoothingSigmasPerLevel(smoothingSigmas);

    registration->SetFixedImage(fixed);
    registration->SetMovingImage(moving);

    try {
        registration->Update();
    } catch (itk::ExceptionObject &e) {
        errorMessage = e.GetDescription();
        return false;
    }

    const TransformType *finalTransform = registration->GetTransform();
    const TransformType::MatrixType matrix = finalTransform->GetMatrix();
    const TransformType::OffsetType offset = finalTransform->GetOffset();

    fixedToMoving->Identity();
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            fixedToMoving->SetElement(row, col, matrix(row, col));
        }
        fixedToMoving->SetElement(row, 3, offset[row]);
    }
    auto tfmWriter = itk::TransformFileWriterTemplate<double>::New();
    tfmWriter->SetInput(finalTransform);
    tfmWriter->SetFileName("rigid.tfm");
    tfmWriter->Update();

    return true;
}
