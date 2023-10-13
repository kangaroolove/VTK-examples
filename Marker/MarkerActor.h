#pragma once

#include <vtkProp3D.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

class vtkCaptionActor2D;
class vtkCylinderSource;
class vtkActor;
class vtkPolyDataMapper;
class vtkLinearTransform;
class vtkCutter;
class vtkPlane;

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
    void setOrigin(double x, double y, double z);
    void setNormal(double x, double y, double z);
    void setText(const std::string& text);
private:
    MarkerActor();
    ~MarkerActor();
    void updateProps();

    double m_bounds[6];
    double m_origin[3];
    double m_normal[3];
    std::string m_text;
    vtkTypeBool hasText;
    vtkSmartPointer<vtkCaptionActor2D> m_captionActor;
    vtkSmartPointer<vtkCylinderSource> m_cylinderSource;
    vtkSmartPointer<vtkActor> m_actor;
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
    vtkSmartPointer<vtkCutter> m_cutter;
    vtkSmartPointer<vtkPlane> m_plane;
};