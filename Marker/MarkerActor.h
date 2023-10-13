#pragma once

#include <vtkProp3D.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

class vtkCaptionActor2D;
class vtkCylinderSource;
class vtkActor;
class vtkPolyDataMapper;
class vtkLinearTransform;

class MarkerActor : public vtkProp3D
{
public:
    vtkTypeMacro(MarkerActor, vtkProp3D);
    static MarkerActor* New();

    void GetBounds(double bounds[6]);
    double *GetBounds() VTK_SIZEHINT(6) override;
    int RenderOpaqueGeometry(vtkViewport *viewport) override;
    int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) override;
    int RenderOverlay(vtkViewport *viewport) override;
    vtkTypeBool HasTranslucentPolygonalGeometry() override;
private:
    MarkerActor();
    ~MarkerActor();
    void updateProps();

    double m_bounds[6];
    vtkTypeBool hasText;
    vtkSmartPointer<vtkCaptionActor2D> m_text;
    vtkSmartPointer<vtkCylinderSource> m_cylinderSource;
    vtkSmartPointer<vtkActor> m_actor;
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
};