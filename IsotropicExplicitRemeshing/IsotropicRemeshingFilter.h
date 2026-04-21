#pragma once
#include <vtkPolyDataAlgorithm.h>

class IsotropicRemeshingFilter : public vtkPolyDataAlgorithm {
public:
    static IsotropicRemeshingFilter* New();
    vtkTypeMacro(IsotropicRemeshingFilter, vtkPolyDataAlgorithm);

    vtkSetMacro(NumberOfClusters, int);
    vtkGetMacro(NumberOfClusters, int);

protected:
    IsotropicRemeshingFilter() : NumberOfClusters(500) {}
    int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
    int NumberOfClusters;
};
