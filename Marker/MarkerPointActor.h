#pragma once

#include <vtkColor.h>
#include <vtkObjectFactory.h>
#include <vtkProp3D.h>
#include <vtkSmartPointer.h>

class vtkCaptionActor2D;
class vtkSphereSource;
class vtkActor;
class vtkPolyDataMapper;
class vtkCutter;
class vtkPlane;
class vtkSTLReader;
class vtkTransformFilter;

class MarkerPointActor : public vtkProp3D {
public:
  vtkTypeMacro(MarkerPointActor, vtkProp3D);
  static MarkerPointActor *New();

  void GetBounds(double bounds[6]);
  double *GetBounds() VTK_SIZEHINT(6) override;
  int RenderOpaqueGeometry(vtkViewport *viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) override;
  int RenderOverlay(vtkViewport *viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  void setOrigin(double x, double y, double z);
  void setOrigin(double origin[3]);
  void setNormal(double x, double y, double z);
  void setNormal(double normal[3]);
  void setText(const std::string &text);
  void setStlFileName(const std::string &fileName);
  void setTextVisible(const bool &visible);
  void setColor(const vtkColor3d &color);

private:
  MarkerPointActor();
  ~MarkerPointActor();
  void updateProps();
  void updateColor();
  bool isDataFromStl();
  void updateOrigin();

  double m_bounds[6];
  double m_origin[3];
  double m_normal[3];
  std::string m_text;
  std::string m_stlFileName;
  bool m_textVisible;
  vtkColor3d m_color;
  vtkSmartPointer<vtkCaptionActor2D> m_captionActor;
  vtkSmartPointer<vtkSphereSource> m_sphereSource;
  vtkSmartPointer<vtkActor> m_actor;
  vtkSmartPointer<vtkPolyDataMapper> m_mapper;
  vtkSmartPointer<vtkSTLReader> m_reader;
  vtkSmartPointer<vtkCutter> m_cutter;
  vtkSmartPointer<vtkPlane> m_plane;
  vtkSmartPointer<vtkActor> m_boundActor;
  vtkSmartPointer<vtkPolyDataMapper> m_boundMapper;
};
