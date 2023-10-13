#include "MarkerActor.h"
#include <vtkCaptionActor2D.h>
#include <vtkCylinderSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkLinearTransform.h>
#include <vtkCutter.h>
#include <vtkPlane.h>

vtkStandardNewMacro(MarkerActor)

void MarkerActor::GetBounds(double bounds[6])
{
    const double *bds = this->GetBounds();
    bounds[0] = bds[0];
    bounds[1] = bds[1];
    bounds[2] = bds[2];
    bounds[3] = bds[3];
    bounds[4] = bds[4];
    bounds[5] = bds[5];
}

int MarkerActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int renderedSomething = 0;

  this->updateProps();

  renderedSomething += this->m_actor->RenderOpaqueGeometry( viewport );

  if ( this->hasText )
    renderedSomething += this->m_captionActor->RenderOpaqueGeometry( viewport );

  renderedSomething = (renderedSomething > 0)?(1):(0);
  return renderedSomething;
}

int MarkerActor::RenderTranslucentPolygonalGeometry(vtkViewport *viewport)
{
  int renderedSomething = 0;

  this->updateProps();

  renderedSomething += this->m_actor->RenderTranslucentPolygonalGeometry( viewport );

  if ( this->hasText )
    renderedSomething += this->m_captionActor->RenderTranslucentPolygonalGeometry( viewport );

  renderedSomething = (renderedSomething > 0)?(1):(0);
  return renderedSomething;
}

int MarkerActor::RenderOverlay(vtkViewport *viewport)
{
    int renderedSomething = 0;

    if ( !this->hasText )
    {
        return renderedSomething;
    }

    this->updateProps();

    renderedSomething += this->m_captionActor->RenderOverlay( viewport );

    renderedSomething = (renderedSomething > 0)?(1):(0);
    return renderedSomething;
}

vtkTypeBool MarkerActor::HasTranslucentPolygonalGeometry()
{
  int result = 0;

  this->updateProps();

  result |= this->m_actor->HasTranslucentPolygonalGeometry();

  if ( this->hasText )
  {
    result |= this->m_captionActor->HasTranslucentPolygonalGeometry();
  }
  return result;
}

void MarkerActor::setOrigin(double x, double y, double z)
{
    m_origin[0] = x;
    m_origin[1] = y;
    m_origin[2] = z;
}

void MarkerActor::setText(const std::string &text)
{
    m_text = text;
}

MarkerActor::MarkerActor() :
    m_bounds{ 0 },
    m_captionActor(vtkSmartPointer<vtkCaptionActor2D>::New()),
    m_cylinderSource(vtkSmartPointer<vtkCylinderSource>::New()),
    m_actor(vtkSmartPointer<vtkActor>::New()),
    m_mapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
    m_cutter(vtkSmartPointer<vtkCutter>::New()),
    m_plane(vtkSmartPointer<vtkPlane>::New()),
    hasText(true),
    m_origin{ 0 }
{
    m_captionActor->ThreeDimensionalLeaderOff();
    m_captionActor->LeaderOff();
    m_captionActor->BorderOff();
    m_captionActor->SetPosition(0, 0);

    m_cylinderSource->SetRadius(5);
    m_cylinderSource->SetResolution(100);
    m_cylinderSource->SetHeight(20.0);

    m_plane->SetOrigin(0, 0, 0);
    m_plane->SetNormal(0, 0, 1);

    m_cutter->SetCutFunction(m_plane);
    m_actor->SetMapper(m_mapper);

    this->updateProps();
}

MarkerActor::~MarkerActor()
{
}

void MarkerActor::updateProps()
{
    m_plane->SetOrigin(m_origin);

    m_cutter->SetInputConnection(m_cylinderSource->GetOutputPort());
    m_mapper->SetInputConnection(m_cutter->GetOutputPort());
    m_mapper->GetInputAlgorithm()->Update();

    if (this->GetUserTransform())
        m_actor->SetUserTransform(nullptr);

    m_captionActor->SetCaption(this->m_text.c_str());
    m_captionActor->SetAttachmentPoint(0, 0, 0);

    vtkLinearTransform* transform = this->GetUserTransform();
    if (transform)
    {
        m_actor->SetUserTransform(transform);

        double newPos[3];
        double *pos = this->m_captionActor->GetAttachmentPoint();

        transform->TransformPoint(pos, newPos);
        m_captionActor->SetAttachmentPoint(newPos);
    }
}

double *MarkerActor::GetBounds()
{
    double bounds[6];
    this->m_actor->GetBounds(bounds);

    for (int i = 0; i < 6; i++)
    {
        m_bounds[i] = bounds[i];
    }

    return this->m_bounds;
}
