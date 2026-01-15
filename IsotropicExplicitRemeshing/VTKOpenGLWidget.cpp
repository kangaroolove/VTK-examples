#include "VTKOpenGLWidget.h"

#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkIsotropicDiscreteRemeshing.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSTLReader.h>
#include <vtkSmartPointer.h>
#include <vtkSurface.h>
#include <vtkSurfaceBase.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_leftRenderer(vtkSmartPointer<vtkRenderer>::New()),
      m_rightRenderer(vtkSmartPointer<vtkRenderer>::New()) {
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
    m_leftRenderer->SetBackground(1.0, 0.0, 0.0);
    m_leftRenderer->SetViewport(0.0, 0.0, 0.5, 1.0);
    m_renderWindow->AddRenderer(m_leftRenderer);

    m_rightRenderer->SetBackground(0.0, 1.0, 0.0);
    m_rightRenderer->SetViewport(0.5, 0, 1.0, 1.0);
    m_renderWindow->AddRenderer(m_rightRenderer);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData() {
    // left side
    vtkNew<vtkSTLReader> reader;
    reader->SetFileName("D:/1.STL");
    reader->Update();

    vtkNew<vtkPolyDataMapper> mapperLeft;
    mapperLeft->SetInputConnection(reader->GetOutputPort());

    vtkNew<vtkActor> actorLeft;
    actorLeft->SetMapper(mapperLeft);
    actorLeft->GetProperty()->EdgeVisibilityOn();

    m_leftRenderer->AddActor(actorLeft);

    // Right side
    vtkNew<vtkSurface> surface;
    surface->CreateFromPolyData(reader->GetOutput());
    surface->GetCellData()->Initialize();
    surface->GetPointData()->Initialize();

    vtkSmartPointer<vtkIsotropicDiscreteRemeshing> remesher =
        vtkSmartPointer<vtkIsotropicDiscreteRemeshing>::New();
    remesher->SetInput(surface);
    remesher->SetNumberOfClusters(500);
    remesher->Remesh();

    vtkNew<vtkPolyDataMapper> mapperRight;
    mapperRight->SetInputData(remesher->GetOutput());

    vtkNew<vtkActor> actorRight;
    actorRight->SetMapper(mapperRight);
    actorRight->GetProperty()->EdgeVisibilityOn();

    m_rightRenderer->AddActor(actorRight);
}
