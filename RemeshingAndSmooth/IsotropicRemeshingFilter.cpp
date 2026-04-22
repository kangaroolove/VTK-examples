#include "IsotropicRemeshingFilter.h"
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIsotropicDiscreteRemeshing.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkSurface.h>

vtkStandardNewMacro(IsotropicRemeshingFilter);

int IsotropicRemeshingFilter::RequestData(vtkInformation*, vtkInformationVector** inputVec,
                                          vtkInformationVector* outputVec) {
    auto* input = vtkPolyData::GetData(inputVec[0]);
    auto* output = vtkPolyData::GetData(outputVec);

    vtkNew<vtkSurface> surface;
    surface->CreateFromPolyData(input);
    surface->GetCellData()->Initialize();
    surface->GetPointData()->Initialize();

    vtkNew<vtkIsotropicDiscreteRemeshing> remesher;
    remesher->SetInput(surface);
    remesher->SetNumberOfClusters(NumberOfClusters);
    remesher->SetConsoleOutput(0);
    remesher->Remesh();

    output->ShallowCopy(remesher->GetOutput());
    return 1;
}
