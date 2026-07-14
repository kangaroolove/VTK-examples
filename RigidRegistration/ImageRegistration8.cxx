/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

// Rigid (6 DOF) registration between two volumes, built for cross-modality
// pairs (e.g. T2 MR and tracked ultrasound) where a mutual-information metric
// is required. See RunRigidRegistration() for the reusable pipeline and
// MakeDefaultT2UltrasoundParams() for the parameters this was originally
// tuned against.

#include "itkImageRegistrationMethodv4.h"
#include "itkMattesMutualInformationImageToImageMetricv4.h"
#include "itkRegistrationParameterScalesFromPhysicalShift.h"
#include "itkEuler3DTransform.h"
#include "itkCenteredTransformInitializer.h"
#include "itkRegularStepGradientDescentOptimizerv4.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkResampleImageFilter.h"
#include "itkSubtractImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkExtractImageFilter.h"
#include "itkImageMaskSpatialObject.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkCommand.h"

#include <iostream>
#include <string>

//  The following section of code implements a Command observer
//  that will monitor the evolution of the registration process.
//
class CommandIterationUpdate : public itk::Command
{
public:
  using Self = CommandIterationUpdate;
  using Superclass = itk::Command;
  using Pointer = itk::SmartPointer<Self>;
  itkNewMacro(Self);

protected:
  CommandIterationUpdate() = default;

public:
  using OptimizerType = itk::RegularStepGradientDescentOptimizerv4<double>;
  using OptimizerPointer = const OptimizerType *;
  void
  Execute(itk::Object * caller, const itk::EventObject & event) override
  {
    Execute((const itk::Object *)caller, event);
  }
  void
  Execute(const itk::Object * object, const itk::EventObject & event) override
  {
    auto optimizer = static_cast<OptimizerPointer>(object);
    if (!itk::IterationEvent().CheckEvent(&event))
    {
      return;
    }
    std::cout << optimizer->GetCurrentIteration() << "   ";
    std::cout << optimizer->GetValue() << "   ";
    std::cout << optimizer->GetCurrentPosition() << std::endl;
  }
};

// Fires once per pyramid level (see itk::MultiResolutionIterationEvent) so
// the per-iteration log above is legible - otherwise it's not obvious from
// the printed iteration count alone that it reset to 0 because a new,
// less-shrunk level started rather than because something went wrong.
class CommandMultiResolutionIterationUpdate : public itk::Command {
public:
    using Self = CommandMultiResolutionIterationUpdate;
    using Superclass = itk::Command;
    using Pointer = itk::SmartPointer<Self>;
    itkNewMacro(Self);

protected:
    CommandMultiResolutionIterationUpdate() = default;

public:
    void Execute(itk::Object *caller, const itk::EventObject &event) override {
        Execute((const itk::Object *)caller, event);
    }
    void Execute(const itk::Object *object,
                 const itk::EventObject &event) override {
        if (!itk::MultiResolutionIterationEvent().CheckEvent(&event)) {
            return;
        }
        std::cout << "-- Starting next resolution level --" << std::endl;
    }
};

constexpr unsigned int Dimension = 3;
using PixelType = float;
using FixedImageType = itk::Image<PixelType, Dimension>;
using MovingImageType = itk::Image<PixelType, Dimension>;

struct RegistrationParams
{
  std::string fixedImagePath;
  std::string movingImagePath;
  std::string outputImagePath = "output.nrrd";
  std::string differenceRegistrationBeforePath = "differenceBefore.nrrd";
  std::string differenceRegistrationAfterPath = "differenceAfter.nrrd";
  std::string differenceRegistrationBeforeImagePath = "differenceBefore.png";
  std::string differenceRegistrationAfterImagePath = "differenceAfter.png";
  std::string registrationImagePath = "output.png";

  // When false, the initial transform's center and translation are derived
  // from image intensity moments (itk::CenteredTransformInitializer), which
  // assumes the fixed and moving volumes cover comparable physical extents.
  // When true, the center and translation are derived from the explicit
  // physical-space centers below instead - needed when that assumption
  // doesn't hold, e.g. a tracked ultrasound volume that only covers a small
  // sector of a whole-head MR. Passing physical points directly (rather than
  // voxel indices) means the caller doesn't need image geometry on hand to
  // specify them.
  bool useExplicitCenters = false;
  FixedImageType::PointType fixedCenter{};
  MovingImageType::PointType movingCenter{};

  // Restricts the metric to the fixed image's actual data region. Needed when
  // the fixed volume has a large empty/constant background (e.g. an
  // ultrasound fan sitting inside a zero-valued bounding box) that would
  // otherwise dominate the joint histogram.
  bool useFixedImageMask = false;

  unsigned int numberOfHistogramBins = 50;
  double metricSamplingPercentage = 1.0;

  unsigned int outputSliceIndex = 90;
};

// The parameters this registration routine was originally built and tuned
// for: a tracked ultrasound scan (fixed) and a T2 MR volume (moving) of the
// same patient, with a mask over the ultrasound fan. The corresponding
// landmark pair was originally picked as voxel indices in each volume; those
// are resolved to physical-space centers here (header-only reads) since
// RegistrationParams takes explicit centers rather than indices.
RegistrationParams
MakeDefaultT2UltrasoundParams()
{
  RegistrationParams params;

  params.fixedImagePath =
    "D:/Standard test-data-V3/Set B - Real Patient/Patient A - Registration/3D-USScan_20201029101446 - 000 NEW.nrrd";
  params.movingImagePath =
    "D:/Standard test-data-V3/Set B - Real Patient/Patient A - Registration/t2/t2.nrrd";

  using FixedImageReaderType = itk::ImageFileReader<FixedImageType>;
  using MovingImageReaderType = itk::ImageFileReader<MovingImageType>;

  FixedImageReaderType::Pointer fixedReader = FixedImageReaderType::New();
  fixedReader->SetFileName(params.fixedImagePath);
  fixedReader->UpdateOutputInformation();

  MovingImageReaderType::Pointer movingReader = MovingImageReaderType::New();
  movingReader->SetFileName(params.movingImagePath);
  movingReader->UpdateOutputInformation();

  FixedImageType::IndexType fixedLandmarkIndex;
  fixedLandmarkIndex[0] = 371;
  fixedLandmarkIndex[1] = 164;
  fixedLandmarkIndex[2] = 65;

  MovingImageType::IndexType movingLandmarkIndex;
  movingLandmarkIndex[0] = 325;
  movingLandmarkIndex[1] = 294;
  movingLandmarkIndex[2] = 9;

  fixedReader->GetOutput()->TransformIndexToPhysicalPoint(
    fixedLandmarkIndex, params.fixedCenter);
  movingReader->GetOutput()->TransformIndexToPhysicalPoint(
    movingLandmarkIndex, params.movingCenter);

  params.useExplicitCenters = true;
  params.useFixedImageMask = true;

  return params;
}

int
RunRigidRegistration(const RegistrationParams & params)
{
  using TransformType = itk::Euler3DTransform<double>;
  using OptimizerType = itk::RegularStepGradientDescentOptimizerv4<double>;
  using MetricType =
    itk::MattesMutualInformationImageToImageMetricv4<FixedImageType,
                                                       MovingImageType>;
  using RegistrationType =
    itk::ImageRegistrationMethodv4<FixedImageType, MovingImageType,
                                    TransformType>;

  MetricType::Pointer       metric = MetricType::New();
  OptimizerType::Pointer    optimizer = OptimizerType::New();
  RegistrationType::Pointer registration = RegistrationType::New();

  // Mutual information only needs a coarse joint histogram to find the
  // statistical dependence between the two modalities' intensities; too many
  // bins makes the histogram sparse and the metric noisy.
  metric->SetNumberOfHistogramBins(params.numberOfHistogramBins);

  registration->SetMetric(metric);
  registration->SetOptimizer(optimizer);

  // Mattes MI is evaluated on a random subset of voxels rather than the full
  // image; this both speeds up each iteration and smooths out the effect of
  // per-modality noise (e.g. ultrasound speckle) on the joint histogram.
  registration->SetMetricSamplingStrategy(
    RegistrationType::MetricSamplingStrategyEnum::RANDOM);
  registration->SetMetricSamplingPercentage(params.metricSamplingPercentage);
  registration->MetricSamplingReinitializeSeed(121213);

  TransformType::Pointer initialTransform = TransformType::New();

  using FixedImageReaderType = itk::ImageFileReader<FixedImageType>;
  using MovingImageReaderType = itk::ImageFileReader<MovingImageType>;

  FixedImageReaderType::Pointer  fixedImageReader = FixedImageReaderType::New();
  MovingImageReaderType::Pointer movingImageReader = MovingImageReaderType::New();

  // Letting each reader auto-detect its ImageIO from the file's extension
  // (instead of forcing a specific ImageIO type on both) is what allows the
  // fixed and moving volumes to be different modalities stored in different
  // file formats (e.g. NRRD ultrasound + MetaImage MR).
  fixedImageReader->SetFileName(params.fixedImagePath);
  movingImageReader->SetFileName(params.movingImagePath);

  registration->SetFixedImage(fixedImageReader->GetOutput());
  registration->SetMovingImage(movingImageReader->GetOutput());

  // UpdateOutputInformation() only reads each file's geometry (origin,
  // spacing, direction), not its pixel data, so this is cheap.
  fixedImageReader->UpdateOutputInformation();
  movingImageReader->UpdateOutputInformation();

  if (params.useExplicitCenters)
  {
    initialTransform->SetCenter(params.fixedCenter);

    TransformType::OutputVectorType initialTranslation;
    for (unsigned int i = 0; i < Dimension; ++i)
    {
      initialTranslation[i] = params.movingCenter[i] - params.fixedCenter[i];
    }
    initialTransform->SetTranslation(initialTranslation);
  }
  else
  {
    using InitializerType =
      itk::CenteredTransformInitializer<TransformType, FixedImageType,
                                          MovingImageType>;
    InitializerType::Pointer initializer = InitializerType::New();
    initializer->SetTransform(initialTransform);
    initializer->SetFixedImage(fixedImageReader->GetOutput());
    initializer->SetMovingImage(movingImageReader->GetOutput());
    initializer->MomentsOn();
    initializer->InitializeTransform();
  }

  // Euler3DTransform stores rotation as three Euler angles (not a versor);
  // the initial rotation is left at identity in both initialization branches
  // above - only the center and translation come from landmarks/moments.
  initialTransform->SetRotation(0.0, 0.0, 0.0);

  registration->SetInitialTransform(initialTransform);

  // Cross-modality volumes rarely share comparable voxel spacing/extent, so
  // a fixed rotation/translation scale ratio is not reliable here. The
  // transform itself is still rotation + translation only (Euler3DTransform,
  // 6 parameters, no scale DOF) - this only tunes the optimizer's internal
  // step-size ratio between the two, estimated from how much each parameter
  // actually moves the image in physical space.
  using ScalesEstimatorType =
    itk::RegistrationParameterScalesFromPhysicalShift<MetricType>;
  ScalesEstimatorType::Pointer scalesEstimator = ScalesEstimatorType::New();
  scalesEstimator->SetMetric(metric);
  scalesEstimator->SetTransformForward(true);
  optimizer->SetScalesEstimator(scalesEstimator);

  optimizer->SetNumberOfIterations(200);
  optimizer->SetLearningRate(1.0);
  optimizer->SetMinimumStepLength(1e-4);
  optimizer->SetReturnBestParametersAndValue(true);

  // Create the Command observer and register it with the optimizer.
  CommandIterationUpdate::Pointer observer = CommandIterationUpdate::New();
  optimizer->AddObserver(itk::IterationEvent(), observer);

  CommandMultiResolutionIterationUpdate::Pointer levelObserver =
      CommandMultiResolutionIterationUpdate::New();
  registration->AddObserver(itk::MultiResolutionIterationEvent(),
                            levelObserver);

  // Coarse-to-fine multi-resolution schedule: registering at full resolution
  // first is unreliable for e.g. MR/ultrasound because ultrasound speckle
  // creates many spurious local optima in the mutual information landscape.
  // Starting heavily shrunk/smoothed lets the optimizer find the broad
  // global alignment cheaply (a shrink-4 level has 1/64th the voxels of full
  // resolution) before refining it on the finer levels; the final level still
  // runs at shrink factor 1 / sigma 0, so accuracy at convergence is
  // unchanged - the earlier levels just get it there in far fewer full-
  // resolution iterations.
  constexpr unsigned int numberOfLevels = 3;
  RegistrationType::ShrinkFactorsArrayType shrinkFactorsPerLevel;
  shrinkFactorsPerLevel.SetSize(numberOfLevels);
  shrinkFactorsPerLevel[0] = 4;
  shrinkFactorsPerLevel[1] = 2;
  shrinkFactorsPerLevel[2] = 1;

  RegistrationType::SmoothingSigmasArrayType smoothingSigmasPerLevel;
  smoothingSigmasPerLevel.SetSize(numberOfLevels);
  smoothingSigmasPerLevel[0] = 2;
  smoothingSigmasPerLevel[1] = 1;
  smoothingSigmasPerLevel[2] = 0;

  registration->SetNumberOfLevels(numberOfLevels);
  registration->SetSmoothingSigmasPerLevel(smoothingSigmasPerLevel);
  registration->SetShrinkFactorsPerLevel(shrinkFactorsPerLevel);

  if (params.useFixedImageMask)
  {
    fixedImageReader->Update();

    using MaskPixelType = unsigned char;
    using MaskImageType = itk::Image<MaskPixelType, Dimension>;
    using MaskThresholdType =
      itk::BinaryThresholdImageFilter<FixedImageType, MaskImageType>;
    MaskThresholdType::Pointer maskThreshold = MaskThresholdType::New();
    maskThreshold->SetInput(fixedImageReader->GetOutput());
    maskThreshold->SetLowerThreshold(1);
    maskThreshold->SetInsideValue(1);
    maskThreshold->SetOutsideValue(0);
    maskThreshold->Update();

    using MaskSpatialObjectType = itk::ImageMaskSpatialObject<Dimension>;
    MaskSpatialObjectType::Pointer fixedImageMask = MaskSpatialObjectType::New();
    fixedImageMask->SetImage(maskThreshold->GetOutput());
    fixedImageMask->Update();
    metric->SetFixedImageMask(fixedImageMask);
  }

  try
  {
    registration->Update();
    std::cout << "Optimizer stop condition: "
              << registration->GetOptimizer()->GetStopConditionDescription()
              << std::endl;
  }
  catch (const itk::ExceptionObject & err)
  {
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
  }

  const TransformType::ParametersType finalParameters =
    registration->GetOutput()->Get()->GetParameters();

  const double       versorX = finalParameters[0];
  const double       versorY = finalParameters[1];
  const double       versorZ = finalParameters[2];
  const double       finalTranslationX = finalParameters[3];
  const double       finalTranslationY = finalParameters[4];
  const double       finalTranslationZ = finalParameters[5];
  const unsigned int numberOfIterations = optimizer->GetCurrentIteration();
  const double       bestValue = optimizer->GetValue();

  // Print out results
  //
  std::cout << std::endl << std::endl;
  std::cout << "Result = " << std::endl;
  std::cout << " versor X      = " << versorX << std::endl;
  std::cout << " versor Y      = " << versorY << std::endl;
  std::cout << " versor Z      = " << versorZ << std::endl;
  std::cout << " Translation X = " << finalTranslationX << std::endl;
  std::cout << " Translation Y = " << finalTranslationY << std::endl;
  std::cout << " Translation Z = " << finalTranslationZ << std::endl;
  std::cout << " Iterations    = " << numberOfIterations << std::endl;
  std::cout << " Metric value  = " << bestValue << std::endl;

  TransformType::Pointer finalTransform = TransformType::New();

  finalTransform->SetFixedParameters(
    registration->GetOutput()->Get()->GetFixedParameters());
  finalTransform->SetParameters(finalParameters);

  TransformType::MatrixType matrix = finalTransform->GetMatrix();
  TransformType::OffsetType offset = finalTransform->GetOffset();
  std::cout << "Matrix = " << std::endl << matrix << std::endl;
  std::cout << "Offset = " << std::endl << offset << std::endl;

  using ResampleFilterType =
    itk::ResampleImageFilter<MovingImageType, FixedImageType>;

  ResampleFilterType::Pointer resampler = ResampleFilterType::New();

  resampler->SetTransform(finalTransform);
  resampler->SetInput(movingImageReader->GetOutput());

  FixedImageType::Pointer fixedImage = fixedImageReader->GetOutput();

  resampler->SetSize(fixedImage->GetLargestPossibleRegion().GetSize());
  resampler->SetOutputOrigin(fixedImage->GetOrigin());
  resampler->SetOutputSpacing(fixedImage->GetSpacing());
  resampler->SetOutputDirection(fixedImage->GetDirection());
  resampler->SetDefaultPixelValue(100);

  using OutputPixelType = unsigned char;
  using OutputImageType = itk::Image<OutputPixelType, Dimension>;
  // Moving image intensities routinely exceed the 0-255 range of unsigned
  // char. A plain CastImageFilter would truncate/wrap those values modulo
  // 256, turning smooth anatomy into spurious bands and contours in the
  // output. RescaleIntensityImageFilter maps the actual intensity range onto
  // 0-255 so the resampled volume is viewable.
  using OutputRescalerType =
    itk::RescaleIntensityImageFilter<FixedImageType, OutputImageType>;
  using WriterType = itk::ImageFileWriter<OutputImageType>;

  WriterType::Pointer         writer = WriterType::New();
  OutputRescalerType::Pointer outputRescaler = OutputRescalerType::New();

  writer->SetFileName(params.outputImagePath);

  outputRescaler->SetOutputMinimum(0);
  outputRescaler->SetOutputMaximum(255);
  outputRescaler->SetInput(resampler->GetOutput());
  writer->SetInput(outputRescaler->GetOutput());
  writer->Update();

  using DifferenceFilterType =
    itk::SubtractImageFilter<FixedImageType, FixedImageType, FixedImageType>;
  DifferenceFilterType::Pointer difference = DifferenceFilterType::New();

  using RescalerType =
    itk::RescaleIntensityImageFilter<FixedImageType, OutputImageType>;
  RescalerType::Pointer intensityRescaler = RescalerType::New();

  intensityRescaler->SetInput(difference->GetOutput());
  intensityRescaler->SetOutputMinimum(0);
  intensityRescaler->SetOutputMaximum(255);

  difference->SetInput1(fixedImageReader->GetOutput());
  difference->SetInput2(resampler->GetOutput());

  resampler->SetDefaultPixelValue(1);

  WriterType::Pointer writer2 = WriterType::New();
  writer2->SetInput(intensityRescaler->GetOutput());

  // Compute the difference image between the fixed and resampled moving
  // image after registration.
  {
    writer2->SetFileName(params.differenceRegistrationAfterPath);
    writer2->Update();
  }

  using IdentityTransformType = itk::IdentityTransform<double, Dimension>;
  IdentityTransformType::Pointer identity = IdentityTransformType::New();
  // Compute the difference image between the fixed and moving image before
  // registration.
  {
    resampler->SetTransform(identity);
    writer2->SetFileName(params.differenceRegistrationBeforePath);
    writer2->Update();
  }

  //  Extract slices from the input volume, and the difference volumes
  //  produced before and after the registration.
  using OutputSliceType = itk::Image<OutputPixelType, 2>;
  using ExtractFilterType =
    itk::ExtractImageFilter<OutputImageType, OutputSliceType>;
  ExtractFilterType::Pointer extractor = ExtractFilterType::New();
  extractor->SetDirectionCollapseToSubmatrix();
  extractor->InPlaceOn();

  FixedImageType::RegionType inputRegion =
    fixedImage->GetLargestPossibleRegion();
  FixedImageType::SizeType  size = inputRegion.GetSize();
  FixedImageType::IndexType start = inputRegion.GetIndex();

  // Select one slice as output
  size[2] = 0;
  start[2] = params.outputSliceIndex;
  FixedImageType::RegionType desiredRegion;
  desiredRegion.SetSize(size);
  desiredRegion.SetIndex(start);
  extractor->SetExtractionRegion(desiredRegion);
  using SliceWriterType = itk::ImageFileWriter<OutputSliceType>;
  SliceWriterType::Pointer sliceWriter = SliceWriterType::New();
  sliceWriter->SetInput(extractor->GetOutput());

  {
    extractor->SetInput(outputRescaler->GetOutput());
    resampler->SetTransform(identity);
    sliceWriter->SetFileName(params.registrationImagePath);
    sliceWriter->Update();
  }
  {
    extractor->SetInput(intensityRescaler->GetOutput());
    resampler->SetTransform(identity);
    sliceWriter->SetFileName(params.differenceRegistrationBeforeImagePath);
    sliceWriter->Update();
  }
  {
    resampler->SetTransform(finalTransform);
    sliceWriter->SetFileName(params.differenceRegistrationAfterImagePath);
    sliceWriter->Update();
  }
  {
    extractor->SetInput(outputRescaler->GetOutput());
    resampler->SetTransform(finalTransform);
    sliceWriter->SetFileName(params.registrationImagePath);
    sliceWriter->Update();
  }

  return EXIT_SUCCESS;
}

void
PrintUsage(const char * programName)
{
  std::cerr
    << "Usage: " << programName << " [fixedImage movingImage outputImage "
    << "[--fixed-center x y z] [--moving-center x y z] [--mask] "
    << "[--histogram-bins N] [--sampling-percentage P]]" << std::endl
    << "  With no arguments, runs the built-in T2/ultrasound example dataset."
    << std::endl
    << "  --fixed-center/--moving-center (both required together) are "
    << "physical-space coordinates (not voxel indices) of a corresponding "
    << "point in each volume; they set the initial transform's center and "
    << "translation directly instead of using image moments - use this when "
    << "the two volumes don't cover comparable physical extents (e.g. a "
    << "partial ultrasound sweep)."
    << std::endl
    << "  --mask restricts the metric to the fixed image's nonzero region - "
    << "use this when the fixed volume has large empty background."
    << std::endl;
}

int
main(int argc, char * argv[])
{
  RegistrationParams params;

  if (argc == 1)
  {
    params = MakeDefaultT2UltrasoundParams();
  }
  else if (argc >= 4)
  {
    params.fixedImagePath = argv[1];
    params.movingImagePath = argv[2];
    params.outputImagePath = argv[3];

    for (int i = 4; i < argc; ++i)
    {
      const std::string arg = argv[i];
      if (arg == "--fixed-center" && i + 3 < argc)
      {
        params.useExplicitCenters = true;
        params.fixedCenter[0] = std::stod(argv[++i]);
        params.fixedCenter[1] = std::stod(argv[++i]);
        params.fixedCenter[2] = std::stod(argv[++i]);
      }
      else if (arg == "--moving-center" && i + 3 < argc)
      {
        params.useExplicitCenters = true;
        params.movingCenter[0] = std::stod(argv[++i]);
        params.movingCenter[1] = std::stod(argv[++i]);
        params.movingCenter[2] = std::stod(argv[++i]);
      }
      else if (arg == "--mask")
      {
        params.useFixedImageMask = true;
      }
      else if (arg == "--histogram-bins" && i + 1 < argc)
      {
        params.numberOfHistogramBins =
          static_cast<unsigned int>(std::stoi(argv[++i]));
      }
      else if (arg == "--sampling-percentage" && i + 1 < argc)
      {
        params.metricSamplingPercentage = std::stod(argv[++i]);
      }
      else
      {
        std::cerr << "Unknown or malformed argument: " << arg << std::endl;
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
      }
    }
  }
  else
  {
    PrintUsage(argv[0]);
    return EXIT_FAILURE;
  }

  return RunRigidRegistration(params);
}
