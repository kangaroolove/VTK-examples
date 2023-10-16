#include "MarkerPointActor.h"
#include <vtkCaptionActor2D.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkLinearTransform.h>
#include <vtkCutter.h>
#include <vtkPlane.h>
#include <vtkSTLReader.h>
#include <vtkSphereSource.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>
#include <vtkTextActor.h>
#include <vtkProperty2D.h>

vtkStandardNewMacro(MarkerPointActor)

void MarkerPointActor::GetBounds(double bounds[6])
{
    const double *bds = this->GetBounds();
    bounds[0] = bds[0];
    bounds[1] = bds[1];
    bounds[2] = bds[2];
    bounds[3] = bds[3];
    bounds[4] = bds[4];
    bounds[5] = bds[5];
}

int MarkerPointActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int renderedSomething = 0;

  this->updateProps();

  renderedSomething += this->m_actor->RenderOpaqueGeometry( viewport );

  if (this->m_textVisible)
    renderedSomething += this->m_captionActor->RenderOpaqueGeometry( viewport );

  renderedSomething = (renderedSomething > 0)?(1):(0);
  return renderedSomething;
}

int MarkerPointActor::RenderTranslucentPolygonalGeometry(vtkViewport *viewport)
{
  int renderedSomething = 0;

  this->updateProps();

  renderedSomething += this->m_actor->RenderTranslucentPolygonalGeometry( viewport );

  if (this->m_textVisible)
    renderedSomething += this->m_captionActor->RenderTranslucentPolygonalGeometry( viewport );

  renderedSomething = (renderedSomething > 0)?(1):(0);
  return renderedSomething;
}

int MarkerPointActor::RenderOverlay(vtkViewport *viewport)
{
    int renderedSomething = 0;

    if (!this->m_textVisible)
    {
        return renderedSomething;
    }

    this->updateProps();

    renderedSomething += this->m_captionActor->RenderOverlay( viewport );

    renderedSomething = (renderedSomething > 0)?(1):(0);
    return renderedSomething;
}

vtkTypeBool MarkerPointActor::HasTranslucentPolygonalGeometry()
{
  int result = 0;

  this->updateProps();

  result |= this->m_actor->HasTranslucentPolygonalGeometry();

  if (this->m_textVisible)
  {
    result |= this->m_captionActor->HasTranslucentPolygonalGeometry();
  }
  return result;
}

void MarkerPointActor::setOrigin(double x, double y, double z)
{
    m_origin[0] = x;
    m_origin[1] = y;
    m_origin[2] = z;
}

void MarkerPointActor::setNormal(double x, double y, double z)
{
    m_normal[0] = x;
    m_normal[1] = y;
    m_normal[2] = z;
}

void MarkerPointActor::setText(const std::string &text)
{
    m_text = text;
}

void MarkerPointActor::setStlFileName(const std::string &fileName)
{
    m_stlFileName = fileName;
}

void MarkerPointActor::setTextVisible(const bool &visible)
{
    m_textVisible = visible;
}

void MarkerPointActor::setColor(const vtkColor3d &color)
{
    m_color = color;
}

MarkerPointActor::MarkerPointActor() :
    m_bounds{ 0 },
    m_captionActor(vtkSmartPointer<vtkCaptionActor2D>::New()),
    m_sphereSource(vtkSmartPointer<vtkSphereSource>::New()),
    m_actor(vtkSmartPointer<vtkActor>::New()),
    m_mapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
    m_cutter(vtkSmartPointer<vtkCutter>::New()),
    m_plane(vtkSmartPointer<vtkPlane>::New()),
    m_reader(vtkSmartPointer<vtkSTLReader>::New()),
    m_textVisible(true),
    m_origin{ 0 },
    m_normal{ 0, 0, 1 },
    m_color(1.0, 0.0, 0.0)
{
    m_text = "T1";

    m_captionActor->ThreeDimensionalLeaderOff();
    m_captionActor->LeaderOff();
    m_captionActor->BorderOff();
    m_captionActor->SetPosition(0, 0);

    m_plane->SetOrigin(m_origin);
    m_plane->SetNormal(m_normal);

    m_cutter->SetCutFunction(m_plane);
    m_actor->SetMapper(m_mapper);

    this->updateProps();
}

MarkerPointActor::~MarkerPointActor()
{
}

void MarkerPointActor::updateProps()
{
    m_plane->SetOrigin(m_origin);
    m_plane->SetNormal(m_normal);

    if (!isDataFromStl())
    {
        m_sphereSource->SetRadius(5);
        m_sphereSource->SetCenter(0, 0, 0);
        m_sphereSource->SetPhiResolution(100);
        m_sphereSource->SetThetaResolution(100);

        m_cutter->SetInputConnection(m_sphereSource->GetOutputPort());
    }
    else 
    {
        m_reader->SetFileName(m_stlFileName.c_str());
        m_reader->Update();

        m_cutter->SetInputConnection(m_reader->GetOutputPort());
    }

    m_mapper->SetInputConnection(m_cutter->GetOutputPort());
    m_mapper->GetInputAlgorithm()->Update();

    updateColor();

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

void MarkerPointActor::updateColor()
{
    m_actor->GetProperty()->SetColor(m_color.GetData());
    m_captionActor->GetTextActor()->GetProperty()->SetColor(m_color.GetData());
}

bool MarkerPointActor::isDataFromStl()
{
    return !m_stlFileName.empty();
}

double *MarkerPointActor::GetBounds()
{
    double bounds[6];
    this->m_actor->GetBounds(bounds);

    for (int i = 0; i < 6; i++)
    {
        m_bounds[i] = bounds[i];
    }

    return this->m_bounds;
}
