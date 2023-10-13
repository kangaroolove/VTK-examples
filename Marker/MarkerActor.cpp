#include "MarkerActor.h"
#include <vtkCaptionActor2D.h>
#include <vtkCylinderSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkLinearTransform.h>

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
    renderedSomething += this->m_text->RenderOpaqueGeometry( viewport );

  renderedSomething = (renderedSomething > 0)?(1):(0);
  return renderedSomething;
}

int MarkerActor::RenderTranslucentPolygonalGeometry(vtkViewport *viewport)
{
  int renderedSomething = 0;

  this->updateProps();

  renderedSomething += this->m_actor->RenderTranslucentPolygonalGeometry( viewport );

  if ( this->hasText )
    renderedSomething += this->m_text->RenderTranslucentPolygonalGeometry( viewport );

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

    renderedSomething += this->m_text->RenderOverlay( viewport );

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
    result |= this->m_text->HasTranslucentPolygonalGeometry();
  }
  return result;
}

MarkerActor::MarkerActor()
    : m_bounds{0}
    , m_text(vtkSmartPointer<vtkCaptionActor2D>::New())
    , m_cylinderSource(vtkSmartPointer<vtkCylinderSource>::New())
    , m_actor(vtkSmartPointer<vtkActor>::New())
    , m_mapper(vtkSmartPointer<vtkPolyDataMapper>::New())
    , hasText(true)
{
    m_text->SetCaption("T1");
    m_text->ThreeDimensionalLeaderOff();
    m_text->LeaderOff();
    m_text->BorderOff();
    m_text->SetPosition(0, 0);

    m_cylinderSource->SetHeight(20.0);
    m_actor->SetMapper(m_mapper);

    this->updateProps();
}

MarkerActor::~MarkerActor()
{
}

void MarkerActor::updateProps()
{
    m_cylinderSource->SetRadius(5);
    m_cylinderSource->SetResolution(100);

    vtkPolyDataMapper::SafeDownCast(this->m_actor->GetMapper())->SetInputConnection(m_cylinderSource->GetOutputPort());

    vtkPolyDataMapper::SafeDownCast(this->m_actor->GetMapper())->GetInputAlgorithm()->Update();

    if (this->GetUserTransform())
        m_actor->SetUserTransform(nullptr);

    m_text->SetAttachmentPoint(0, 0, 0);

    vtkLinearTransform* transform = this->GetUserTransform();
    if (transform)
    {
        m_actor->SetUserTransform(transform);

        double newPos[3];
        double *pos = this->m_text->GetAttachmentPoint();

        transform->TransformPoint(pos, newPos);
        m_text->SetAttachmentPoint(newPos);
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
