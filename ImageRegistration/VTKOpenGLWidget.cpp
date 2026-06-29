#include "VTKOpenGLWidget.h"

#include <itkCenteredTransformInitializer.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageRegistrationMethodv4.h>
#include <itkImageToVTKImageFilter.h>
#include <itkMattesMutualInformationImageToImageMetricv4.h>
#include <itkMultiThreaderBase.h>
#include <itkNrrdImageIO.h>
#include <itkOrientImageFilter.h>
#include <itkRegistrationParameterScalesFromPhysicalShift.h>
#include <itkRegularStepGradientDescentOptimizerv4.h>
#include <itkTransformFileWriter.h>
#include <itkVersorRigid3DTransform.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageEllipsoidSource.h>
#include <vtkImageMapper3D.h>
#include <vtkImageProperty.h>
#include <vtkImageReslice.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLineSource.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>

#include <QMouseEvent>
#include <algorithm>

namespace {

// Reslice axes direction cosines (view x, view y, slice normal) per orientation.
// The camera looks along -normal with view y up, giving the radiological
// display convention (world is LPS):
//   Sagittal:   (p.y, p.z),  viewed from the left,  anterior on screen left
//   Coronal:    (p.x, p.z),  viewed from the front, patient left on screen right
//   Transverse: (p.x, -p.y), viewed from the feet,  anterior at screen top
const double kResliceAxes[3][9] = {
    {0, 1, 0, 0, 0, 1, 1, 0, 0},   // Sagittal
    {1, 0, 0, 0, 0, 1, 0, -1, 0},  // Coronal
    {1, 0, 0, 0, -1, 0, 0, 0, -1}, // Transverse
};

// Crosshair lines lie slightly above the slice so they are always visible.
const double kCrosshairZ = 1.0;

// The moving image sits slightly above the fixed one so the blending order is
// deterministic and there is no z-fighting between the two slices.
const double kMovingImageZ = 0.1;

// Each plane has a fixed color; a view draws the lines of the two other planes.
const double kPlaneColors[3][3] = {
    {1.0, 1.0, 0.0}, // Sagittal: yellow
    {1.0, 0.0, 0.0}, // Coronal: red
    {0.0, 1.0, 0.0}, // Transverse: green
};

vtkSmartPointer<vtkActor> createLineActor(vtkLineSource *line, const double color[3]) {
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(line->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(color[0], color[1], color[2]);
    actor->GetProperty()->SetLineWidth(1.5);
    return actor;
}

// A fixed seed for the metric's point sampler. Combined with a regular
// (non-random) sampling strategy and a fixed work-unit count, the registration
// is fully reproducible: the same fixed/moving images always produce the same
// transform.
const int kRegistrationSeed = 121212;

// The registration runs multi-threaded for speed, but with a *fixed* number of
// work units rather than the hardware default. ITK v4 metrics split the sample
// set into this many chunks and sum the per-chunk results in chunk order, so a
// constant count makes the floating-point reduction order identical on every
// run (and on every machine), which is what keeps the result reproducible.
const int kRegistrationWorkUnits = 8;

typedef itk::Image<float, 3> RegImageType;

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
                          vtkMatrix4x4 *fixedToMoving, std::string &errorMessage) {
    typedef itk::VersorRigid3DTransform<double> TransformType;
    typedef itk::RegularStepGradientDescentOptimizerv4<double> OptimizerType;
    typedef itk::MattesMutualInformationImageToImageMetricv4<RegImageType, RegImageType>
        MetricType;
    typedef itk::ImageRegistrationMethodv4<RegImageType, RegImageType, TransformType>
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

    // Random sampling seeded for reproducibility. 10% gives a stable MI gradient
    // so the optimizer needs fewer iterations from the pre-aligned starting point.
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

bool runRigidRegistration2(RegImageType *fixed, RegImageType *moving,
                           const double *fixedCenter,
                           const double *movingCenter,
                           vtkMatrix4x4 *fixedToMoving,
                           std::string &errorMessage) {
    // 1. Define Components
    using TransformType = itk::VersorRigid3DTransform<double>;
    using MetricType =
        itk::MattesMutualInformationImageToImageMetricv4<RegImageType,
                                                         RegImageType>;
    using OptimizerType = itk::RegularStepGradientDescentOptimizerv4<double>;
    using RegistrationType =
        itk::ImageRegistrationMethodv4<RegImageType, RegImageType,
                                       TransformType>;

    auto transform = TransformType::New();
    auto metric = MetricType::New();
    auto optimizer = OptimizerType::New();
    auto registration = RegistrationType::New();

    // 2. Initialize the Transform using the World Points
    TransformType::InputPointType center;
    TransformType::OutputVectorType initialTranslation;
    for (int i = 0; i < 3; ++i) {
        center[i] = fixedCenter[i];
        initialTranslation[i] = movingCenter[i] - fixedCenter[i];
    }
    transform->SetIdentity();
    transform->SetCenter(center);
    transform->SetTranslation(initialTranslation);

    // 3. Configure the Metric (Multi-modal)
    metric->SetNumberOfHistogramBins(50);
    // Optional but recommended for mutual information: use random sampling
    metric->SetUseMovingImageGradientFilter(false);
    metric->SetUseFixedImageGradientFilter(false);

    // 4. Configure the Optimizer
    // Scales are critical! Rotation parameters are [-1, 1], translation is in
    // millimeters.
    using OptimizerScalesType = OptimizerType::ScalesType;
    OptimizerScalesType optimizerScales(transform->GetNumberOfParameters());

    const double translationScale =
        1.0 / 1000.0;                       // Penalize large translations
    optimizerScales[0] = 1.0;               // Versor X
    optimizerScales[1] = 1.0;               // Versor Y
    optimizerScales[2] = 1.0;               // Versor Z
    optimizerScales[3] = translationScale;  // Translation X
    optimizerScales[4] = translationScale;  // Translation Y
    optimizerScales[5] = translationScale;  // Translation Z

    optimizer->SetScales(optimizerScales);
    optimizer->SetLearningRate(1.0);
    optimizer->SetMinimumStepLength(0.001);
    optimizer->SetNumberOfIterations(200);

    // 5. Setup Registration Pipeline
    registration->SetFixedImage(fixed);
    registration->SetMovingImage(moving);
    registration->SetMetric(metric);
    registration->SetOptimizer(optimizer);

    // Set the initial transform we calculated
    registration->SetInitialTransform(transform);

    // Optimize the transform parameters in place
    registration->SetInPlace(true);

    // 6. Execute Registration
    try {
        std::cout << "Starting Registration..." << std::endl;
        registration->Update();
        std::cout << "Registration Complete!" << std::endl;
    } catch (itk::ExceptionObject &err) {
        errorMessage = err.GetDescription();
        return false;
    }

    // 7. Store result into fixedToMoving
    const TransformType::MatrixType matrix = transform->GetMatrix();
    const TransformType::OffsetType offset = transform->GetOffset();

    fixedToMoving->Identity();
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            fixedToMoving->SetElement(row, col, matrix(row, col));
        }
        fixedToMoving->SetElement(row, 3, offset[row]);
    }

    return true;
}

} // namespace

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_activeViewIndex(-1) {
    m_opacities[FixedImage] = 1.0;
    // Semi-transparent by default so the fixed image shows through.
    m_opacities[MovingImage] = 0.5;
    m_hasCenter[FixedImage] = false;
    m_hasCenter[MovingImage] = false;
    for (int i = 0; i < 3; ++i) {
        m_views[i].renderer = vtkSmartPointer<vtkRenderer>::New();
        m_views[i].orientation = i;
    }
    initialize();
    createTestData();
}

void VTKOpenGLWidget::initialize() {
    // 2x2 layout: transverse top-left, sagittal top-right, coronal bottom-left
    m_views[Transverse].renderer->SetViewport(0.0, 0.5, 0.5, 1.0);
    m_views[Sagittal].renderer->SetViewport(0.5, 0.5, 1.0, 1.0);
    m_views[Coronal].renderer->SetViewport(0.0, 0.0, 0.5, 0.5);

    m_views[Transverse].renderer->SetBackground(0.0, 0.0, 0.0);
    m_views[Sagittal].renderer->SetBackground(0.05, 0.05, 0.05);
    m_views[Coronal].renderer->SetBackground(0.1, 0.1, 0.1);

    for (int i = 0; i < 3; ++i) {
        m_renderWindow->AddRenderer(m_views[i].renderer);
    }
    SetRenderWindow(m_renderWindow);

    vtkNew<vtkInteractorStyleImage> style;
    GetInteractor()->SetInteractorStyle(style);
}

void VTKOpenGLWidget::createTestData() {}

bool VTKOpenGLWidget::loadImage(const QString &fileName, ImageRole role,
                                QString &errorMessage) {
    typedef itk::Image<float, 3> ImageType;

    const std::string path = fileName.toStdString();

    itk::NrrdImageIO::Pointer io = itk::NrrdImageIO::New();
    if (!io->CanReadFile(path.c_str())) {
        errorMessage = tr("Only NRRD images (*.nrrd, *.nhdr) are supported:\n%1")
                           .arg(fileName);
        return false;
    }

    // ITK converts the anatomical spaces a NRRD header can declare (RAS,
    // LAS, ...) to LPS world coordinates, so origin and direction are
    // expressed in LPS after reading.
    itk::ImageFileReader<ImageType>::Pointer reader =
        itk::ImageFileReader<ImageType>::New();
    reader->SetImageIO(io);
    reader->SetFileName(path);

    // The world coordinates are LPS, but the voxel grid still uses the
    // file's own axis order, so the direction matrix may hold axis flips
    // and permutations (e.g. an RAS-ordered file reads with direction
    // diag(-1,-1,1)). Reorient the voxels so the grid axes follow RAI;
    // afterwards the direction is identity unless the scan is oblique.
    typedef itk::OrientImageFilter<ImageType, ImageType> OrientFilterType;
    OrientFilterType::Pointer orienter = OrientFilterType::New();
    orienter->UseImageDirectionOn();
    orienter->SetDesiredCoordinateOrientation(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAI);
    orienter->SetInput(reader->GetOutput());
    try {
        orienter->Update();
    } catch (itk::ExceptionObject &e) {
        errorMessage = tr("Failed to read image:\n%1\n%2")
                           .arg(fileName)
                           .arg(QString::fromLatin1(e.GetDescription()));
        return false;
    }

    ImageType::Pointer itkImage = orienter->GetOutput();

    // The vtkImageData origin is zeroed so image coordinates are simply
    // voxel * spacing from (0,0,0). The real LPS origin and direction are
    // encoded in imageToWorld: world = D * p_image + origin.
    const ImageType::DirectionType direction = itkImage->GetDirection();
    const ImageType::PointType origin = itkImage->GetOrigin();
    vtkNew<vtkMatrix4x4> imageToWorld;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            imageToWorld->SetElement(row, col, direction(row, col));
        }
        imageToWorld->SetElement(row, 3, origin[row]);
    }

    typedef itk::ImageToVTKImageFilter<ImageType> ConnectorType;
    ConnectorType::Pointer connector = ConnectorType::New();
    connector->SetInput(itkImage);
    connector->Update();

    vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
    image->DeepCopy(connector->GetOutput());
    image->SetOrigin(0.0, 0.0, 0.0);

    setImage(role, image, imageToWorld);
    // Keep the oriented ITK image (in LPS physical space) for registration.
    m_layers[role].itkImage = itkImage;
    m_renderWindow->Render();
    return true;
}

void VTKOpenGLWidget::setImage(ImageRole role, vtkImageData *image,
                               vtkMatrix4x4 *imageToWorld) {
    m_layers[role].image = image;
    m_layers[role].imageToWorld = imageToWorld;
    updateWorldBounds();

    m_crosshair[0] = 0.5 * (m_worldBounds[0] + m_worldBounds[1]);
    m_crosshair[1] = 0.5 * (m_worldBounds[2] + m_worldBounds[3]);
    m_crosshair[2] = 0.5 * (m_worldBounds[4] + m_worldBounds[5]);

    for (int i = 0; i < 3; ++i) {
        setupSliceView(m_views[i]);
    }
}

bool VTKOpenGLWidget::hasImage() const {
    return m_layers[FixedImage].image || m_layers[MovingImage].image;
}

void VTKOpenGLWidget::setImageOpacity(ImageRole role, double opacity) {
    m_opacities[role] = std::min(std::max(opacity, 0.0), 1.0);
    for (int i = 0; i < 3; ++i) {
        if (m_views[i].actors[role]) {
            m_views[i].actors[role]->GetProperty()->SetOpacity(m_opacities[role]);
        }
    }
    m_renderWindow->Render();
}

double VTKOpenGLWidget::imageOpacity(ImageRole role) const {
    return m_opacities[role];
}

void VTKOpenGLWidget::updateWorldBounds() {
    // World bounds are the axis-aligned box around the transformed corners of
    // every loaded volume's image-space bounding box.
    bool first = true;
    for (int layer = 0; layer < 2; ++layer) {
        vtkImageData *image = m_layers[layer].image;
        if (!image) {
            continue;
        }
        double imageBounds[6];
        image->GetBounds(imageBounds);

        for (int corner = 0; corner < 8; ++corner) {
            const double point[4] = {imageBounds[corner & 1],
                                     imageBounds[2 + ((corner >> 1) & 1)],
                                     imageBounds[4 + ((corner >> 2) & 1)], 1.0};
            double world[4];
            m_layers[layer].imageToWorld->MultiplyPoint(point, world);
            for (int axis = 0; axis < 3; ++axis) {
                if (first) {
                    m_worldBounds[2 * axis] = world[axis];
                    m_worldBounds[2 * axis + 1] = world[axis];
                } else {
                    m_worldBounds[2 * axis] = std::min(m_worldBounds[2 * axis], world[axis]);
                    m_worldBounds[2 * axis + 1] = std::max(m_worldBounds[2 * axis + 1], world[axis]);
                }
            }
            first = false;
        }
    }
}

void VTKOpenGLWidget::setupSliceView(SliceView &view) {
    // A new image replaces the previous slice and crosshair actors.
    view.renderer->RemoveAllViewProps();

    for (int layer = 0; layer < 2; ++layer) {
        view.reslices[layer] = nullptr;
        view.actors[layer] = nullptr;

        vtkImageData *image = m_layers[layer].image;
        if (!image) {
            continue;
        }

        vtkSmartPointer<vtkImageReslice> reslice =
            vtkSmartPointer<vtkImageReslice>::New();
        reslice->SetInputData(image);
        reslice->SetOutputDimensionality(2);
        const double *axes = kResliceAxes[view.orientation];
        reslice->SetResliceAxesDirectionCosines(axes[0], axes[1], axes[2],
                                                axes[3], axes[4], axes[5],
                                                axes[6], axes[7], axes[8]);

        // The reslice axes are world-aligned (LPS) planes; the reslice
        // transform maps those world coordinates into the image's own grid,
        // applying the volume's rotation and origin.
        vtkNew<vtkMatrix4x4> worldToImage;
        vtkMatrix4x4::Invert(m_layers[layer].imageToWorld, worldToImage);
        vtkNew<vtkTransform> resliceTransform;
        resliceTransform->SetMatrix(worldToImage);
        reslice->SetResliceTransform(resliceTransform);
        reslice->AutoCropOutputOn();
        reslice->SetInterpolationModeToLinear();

        double range[2];
        image->GetScalarRange(range);
        if (range[1] <= range[0]) {
            range[1] = range[0] + 1.0;
        }
        reslice->SetBackgroundLevel(range[0]);
        view.reslices[layer] = reslice;

        vtkSmartPointer<vtkImageActor> actor =
            vtkSmartPointer<vtkImageActor>::New();
        actor->GetMapper()->SetInputConnection(reslice->GetOutputPort());
        actor->GetProperty()->SetColorWindow(range[1] - range[0]);
        actor->GetProperty()->SetColorLevel(0.5 * (range[0] + range[1]));
        actor->GetProperty()->SetOpacity(m_opacities[layer]);
        if (layer == MovingImage) {
            actor->SetPosition(0.0, 0.0, kMovingImageZ);
        }
        view.actors[layer] = actor;
        view.renderer->AddViewProp(actor);
    }

    updateResliceOrigin(view);

    // The in-plane components of the reslice origin never change, so these
    // bounds stay valid as the crosshair moves. With both images loaded the
    // view covers the union of their slice bounds.
    bool first = true;
    for (int layer = 0; layer < 2; ++layer) {
        if (!view.reslices[layer]) {
            continue;
        }
        view.reslices[layer]->Update();
        double bounds[6];
        view.reslices[layer]->GetOutput()->GetBounds(bounds);
        for (int i = 0; i < 6; i += 2) {
            if (first) {
                view.bounds[i] = bounds[i];
                view.bounds[i + 1] = bounds[i + 1];
            } else {
                view.bounds[i] = std::min(view.bounds[i], bounds[i]);
                view.bounds[i + 1] = std::max(view.bounds[i + 1], bounds[i + 1]);
            }
        }
        first = false;
    }
    if (first) {
        return; // no image loaded
    }

    // Lines are colored by the plane they represent in this view.
    int horizontalPlane = 0;
    int verticalPlane = 0;
    switch (view.orientation) {
    case Sagittal:
        horizontalPlane = Transverse;
        verticalPlane = Coronal;
        break;
    case Coronal:
        horizontalPlane = Transverse;
        verticalPlane = Sagittal;
        break;
    case Transverse:
        horizontalPlane = Coronal;
        verticalPlane = Sagittal;
        break;
    }

    view.horizontalLine = vtkSmartPointer<vtkLineSource>::New();
    view.verticalLine = vtkSmartPointer<vtkLineSource>::New();
    updateCrosshairLines(view);
    view.renderer->AddActor(createLineActor(view.horizontalLine, kPlaneColors[horizontalPlane]));
    view.renderer->AddActor(createLineActor(view.verticalLine, kPlaneColors[verticalPlane]));

    const double centerX = 0.5 * (view.bounds[0] + view.bounds[1]);
    const double centerY = 0.5 * (view.bounds[2] + view.bounds[3]);
    vtkCamera *camera = view.renderer->GetActiveCamera();
    camera->ParallelProjectionOn();
    camera->SetFocalPoint(centerX, centerY, 0.0);
    camera->SetPosition(centerX, centerY, 500.0);
    camera->SetViewUp(0.0, 1.0, 0.0);
}

void VTKOpenGLWidget::updateResliceOrigin(SliceView &view) {
    double origin[3] = {0.0, 0.0, 0.0};
    switch (view.orientation) {
    case Sagittal:
        origin[0] = m_crosshair[0];
        break;
    case Coronal:
        origin[1] = m_crosshair[1];
        break;
    case Transverse:
        origin[2] = m_crosshair[2];
        break;
    }
    for (int layer = 0; layer < 2; ++layer) {
        if (view.reslices[layer]) {
            view.reslices[layer]->SetResliceAxesOrigin(origin[0], origin[1],
                                                       origin[2]);
        }
    }
}

void VTKOpenGLWidget::crosshairToViewCoords(const SliceView &view, double &x,
                                            double &y) const {
    switch (view.orientation) {
    case Sagittal:
        x = m_crosshair[1];
        y = m_crosshair[2];
        break;
    case Coronal:
        x = m_crosshair[0];
        y = m_crosshair[2];
        break;
    case Transverse:
        x = m_crosshair[0];
        y = -m_crosshair[1];
        break;
    }
}

void VTKOpenGLWidget::updateCrosshairLines(SliceView &view) {
    double x = 0.0;
    double y = 0.0;
    crosshairToViewCoords(view, x, y);

    view.horizontalLine->SetPoint1(view.bounds[0], y, kCrosshairZ);
    view.horizontalLine->SetPoint2(view.bounds[1], y, kCrosshairZ);
    view.verticalLine->SetPoint1(x, view.bounds[2], kCrosshairZ);
    view.verticalLine->SetPoint2(x, view.bounds[3], kCrosshairZ);
}

void VTKOpenGLWidget::setCrosshairPosition(double x, double y, double z) {
    if (!hasImage()) {
        return;
    }
    m_crosshair[0] = std::min(std::max(x, m_worldBounds[0]), m_worldBounds[1]);
    m_crosshair[1] = std::min(std::max(y, m_worldBounds[2]), m_worldBounds[3]);
    m_crosshair[2] = std::min(std::max(z, m_worldBounds[4]), m_worldBounds[5]);

    for (int i = 0; i < 3; ++i) {
        updateResliceOrigin(m_views[i]);
        updateCrosshairLines(m_views[i]);
    }
    m_renderWindow->Render();
}

bool VTKOpenGLWidget::setProstateCenter(ImageRole role) {
    if (!m_layers[role].image || !m_layers[role].imageToWorld) {
        return false;
    }
    // The crosshair is in world coordinates; convert it to the image's own
    // physical coordinates (which is what the registration works in) by
    // inverting the layer's image-to-world matrix. This stays correct even if
    // the moving image has already been repositioned by a previous registration.
    vtkNew<vtkMatrix4x4> worldToImage;
    vtkMatrix4x4::Invert(m_layers[role].imageToWorld, worldToImage);
    const double world[4] = {m_crosshair[0], m_crosshair[1], m_crosshair[2], 1.0};
    double physical[4];
    worldToImage->MultiplyPoint(world, physical);
    m_centers[role][0] = physical[0];
    m_centers[role][1] = physical[1];
    m_centers[role][2] = physical[2];
    m_hasCenter[role] = true;
    return true;
}

bool VTKOpenGLWidget::setProstateCenter(ImageRole role, int i, int j, int k) {
    if (!m_layers[role].itkImage) {
        return false;
    }
    itk::Image<float, 3>::IndexType idx;
    idx[0] = i;
    idx[1] = j;
    idx[2] = k;
    itk::Image<float, 3>::PointType physical;
    m_layers[role].itkImage->TransformIndexToPhysicalPoint(idx, physical);
    m_centers[role][0] = physical[0];
    m_centers[role][1] = physical[1];
    m_centers[role][2] = physical[2];
    m_hasCenter[role] = true;
    setCrosshairPosition(physical[0], physical[1], physical[2]);
    return true;
}

bool VTKOpenGLWidget::getImageDimensions(ImageRole role, int dims[3]) const {
    if (!m_layers[role].image) {
        return false;
    }
    m_layers[role].image->GetDimensions(dims);
    return true;
}

bool VTKOpenGLWidget::hasProstateCenter(ImageRole role) const {
    return m_hasCenter[role];
}

bool VTKOpenGLWidget::registerMovingToFixed(QString &errorMessage) {
    RegImageType *fixed = m_layers[FixedImage].itkImage;
    RegImageType *moving = m_layers[MovingImage].itkImage;
    if (!fixed || !moving) {
        errorMessage = tr("Both a fixed (ultrasound) and a moving (MRI) image "
                          "must be loaded before registering.");
        return false;
    }

    // Use the prostate landmarks only when both have been set; otherwise let the
    // registration fall back to aligning the geometric centers.
    const double *fixedCenter =
        m_hasCenter[FixedImage] ? m_centers[FixedImage] : nullptr;
    const double *movingCenter =
        m_hasCenter[MovingImage] ? m_centers[MovingImage] : nullptr;

    // Pin the work-unit count to a fixed value (rather than the hardware
    // default) for the duration. This keeps the threaded floating-point
    // reductions in a fixed order -- so the result is identical on every run --
    // while still running in parallel for speed. The previous setting is
    // restored afterwards.
    const itk::ThreadIdType previousThreads =
        itk::MultiThreaderBase::GetGlobalDefaultNumberOfThreads();
    itk::MultiThreaderBase::SetGlobalDefaultNumberOfThreads(kRegistrationWorkUnits);

    vtkNew<vtkMatrix4x4> fixedToMoving;
    std::string regError;
    const bool ok = runRigidRegistration2(
        fixed, moving, fixedCenter, movingCenter, fixedToMoving, regError);

    itk::MultiThreaderBase::SetGlobalDefaultNumberOfThreads(previousThreads);

    if (!ok) {
        errorMessage = tr("Registration failed:\n%1")
                           .arg(QString::fromStdString(regError));
        return false;
    }

    // The registration transform T maps a fixed-image point to the
    // corresponding moving-image point. A moving voxel currently shown at world
    // point p therefore belongs at the fixed-aligned world point T^-1(p), so we
    // pre-multiply the moving layer's image-to-world matrix by T^-1.
    vtkNew<vtkMatrix4x4> movingToFixed;
    vtkMatrix4x4::Invert(fixedToMoving, movingToFixed);

    vtkNew<vtkMatrix4x4> alignedImageToWorld;
    vtkMatrix4x4::Multiply4x4(movingToFixed, m_layers[MovingImage].imageToWorld,
                              alignedImageToWorld);
    m_layers[MovingImage].imageToWorld->DeepCopy(alignedImageToWorld);

    // Rebuild the slice views with the repositioned moving image.
    updateWorldBounds();
    for (int i = 0; i < 3; ++i) {
        setupSliceView(m_views[i]);
    }
    // Keep the crosshair inside the (possibly changed) world bounds.
    setCrosshairPosition(m_crosshair[0], m_crosshair[1], m_crosshair[2]);
    m_renderWindow->Render();
    return true;
}

bool VTKOpenGLWidget::event(QEvent *event) {
    // Intercept before QVTKOpenGLNativeWidget forwards the event to the VTK
    // interactor, so the left button is reserved for the crosshair.
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        if (handleCrosshairMouseEvent(static_cast<QMouseEvent *>(event))) {
            return true;
        }
        break;
    default:
        break;
    }
    return QVTKOpenGLNativeWidget::event(event);
}

bool VTKOpenGLWidget::handleCrosshairMouseEvent(QMouseEvent *event) {
    // No image means no slice views or crosshair to interact with.
    if (!hasImage()) {
        return false;
    }
    if (event->type() == QEvent::MouseButtonPress) {
        if (event->button() != Qt::LeftButton) {
            return false;
        }
        int x = 0;
        int y = 0;
        toVtkDisplayCoords(event, x, y);

        m_activeViewIndex = -1;
        vtkRenderer *poked = GetInteractor()->FindPokedRenderer(x, y);
        for (int i = 0; i < 3; ++i) {
            if (m_views[i].renderer.GetPointer() == poked) {
                m_activeViewIndex = i;
            }
        }
        if (m_activeViewIndex < 0) {
            return false;
        }
        moveCrosshairToDisplayPoint(x, y);
        return true;
    }

    if (event->type() == QEvent::MouseMove) {
        if (m_activeViewIndex < 0 || !(event->buttons() & Qt::LeftButton)) {
            return false;
        }
        int x = 0;
        int y = 0;
        toVtkDisplayCoords(event, x, y);
        moveCrosshairToDisplayPoint(x, y);
        return true;
    }

    // MouseButtonRelease
    if (event->button() == Qt::LeftButton && m_activeViewIndex >= 0) {
        m_activeViewIndex = -1;
        return true;
    }
    return false;
}

void VTKOpenGLWidget::toVtkDisplayCoords(QMouseEvent *event, int &x, int &y) const {
    // Qt has its origin top-left; VTK display coordinates start bottom-left
    // and may be scaled on high-DPI screens.
    const int *size = m_renderWindow->GetSize();
    const double scaleX = static_cast<double>(size[0]) / std::max(1, width());
    const double scaleY = static_cast<double>(size[1]) / std::max(1, height());
    x = static_cast<int>(event->x() * scaleX);
    y = size[1] - 1 - static_cast<int>(event->y() * scaleY);
}

void VTKOpenGLWidget::moveCrosshairToDisplayPoint(int x, int y) {
    SliceView &view = m_views[m_activeViewIndex];
    vtkRenderer *renderer = view.renderer;

    // Project the display point onto the slice plane (z = 0 in view coords).
    double focalPoint[3];
    renderer->GetActiveCamera()->GetFocalPoint(focalPoint);
    renderer->SetWorldPoint(focalPoint[0], focalPoint[1], focalPoint[2], 1.0);
    renderer->WorldToDisplay();
    const double displayZ = renderer->GetDisplayPoint()[2];

    renderer->SetDisplayPoint(x, y, displayZ);
    renderer->DisplayToWorld();
    double world[4];
    renderer->GetWorldPoint(world);

    double pos[3] = {m_crosshair[0], m_crosshair[1], m_crosshair[2]};
    switch (view.orientation) {
    case Sagittal:
        pos[1] = world[0];
        pos[2] = world[1];
        break;
    case Coronal:
        pos[0] = world[0];
        pos[2] = world[1];
        break;
    case Transverse:
        pos[0] = world[0];
        pos[1] = -world[1];
        break;
    }
    setCrosshairPosition(pos[0], pos[1], pos[2]);
}
