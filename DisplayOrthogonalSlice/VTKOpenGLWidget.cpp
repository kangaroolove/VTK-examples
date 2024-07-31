#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkNrrdReader.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
{
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget()
{

}

void VTKOpenGLWidget::initialize()
{
    for (auto& renderer : m_renderers)
    {
        renderer = vtkSmartPointer<vtkRenderer>::New();
        m_renderWindow->AddRenderer(renderer);
    }

    m_renderers[0]->SetViewport(0, 0, 0.33, 1);
    m_renderers[0]->SetBackground(1.0, 0, 0);

    m_renderers[1]->SetViewport(0.33, 0, 0.66, 1);
    m_renderers[1]->SetBackground(0, 1.0, 0);

    m_renderers[2]->SetViewport(0.66, 0, 1.0, 1);
    m_renderers[2]->SetBackground(0, 0.0, 1.0);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData()
{
    std::string fileName = "D:/MRI.nrrd";
    vtkNew<vtkNrrdReader> reader;
    reader->SetFileName(fileName.data());
    reader->Update();

    int extent[6] = { 0 };
    reader->GetDataExtent(extent);

    int middleExtentX = (extent[0] + extent[1]) / 2;
    int middleExtentY = (extent[2] + extent[3]) / 2;
    int middleExtentZ = (extent[4] + extent[5]) / 2;

    std::array<vtkSmartPointer<vtkImageActor>, 3> actors;
    for (auto& actor : actors)
    {
        actor = vtkSmartPointer<vtkImageActor>::New();
        actor->SetInputData(reader->GetOutput());
    }

    actors[0]->SetDisplayExtent(middleExtentX, middleExtentX, extent[2], extent[3], extent[4], extent[5]);
    actors[1]->SetDisplayExtent(extent[0], extent[1], middleExtentY, middleExtentY, extent[4], extent[5]);
    actors[2]->SetDisplayExtent(extent[0], extent[1], extent[2], extent[3], middleExtentZ, middleExtentZ);

    for (int i = 0; i < 3; i++)
        m_renderers[i]->AddViewProp(actors[i]);
}
