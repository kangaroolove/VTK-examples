#include "VTKOpenGLWidget.h"

#include <vtkActor.h>
#include <vtkCameraPass.h>
#include <vtkConeSource.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkHiddenLineRemovalPass.h>
#include "IsotropicRemeshingFilter.h"
#include <vtkLightsPass.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderPassCollection.h>
#include <vtkRenderer.h>
#include <vtkSTLReader.h>
#include <vtkSequencePass.h>
#include <vtkSmartPointer.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_leftRenderer(vtkSmartPointer<vtkRenderer>::New()),
      m_rightRenderer(vtkSmartPointer<vtkRenderer>::New()),
      m_middleRenderer(vtkSmartPointer<vtkRenderer>::New()) {
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
    m_leftRenderer->SetBackground(1.0, 0.0, 0.0);
    m_leftRenderer->SetViewport(0.0, 0.0, 0.33, 1.0);
    m_renderWindow->AddRenderer(m_leftRenderer);

    m_middleRenderer->SetBackground(0.0, 1.0, 0.0);
    m_middleRenderer->SetViewport(0.33, 0.0, 0.66, 1.0);
    m_renderWindow->AddRenderer(m_middleRenderer);

    m_rightRenderer->SetBackground(0.0, 0.0, 0.0);
    m_rightRenderer->SetViewport(0.66, 0, 1.0, 1.0);
    m_renderWindow->AddRenderer(m_rightRenderer);
    SetRenderWindow(m_renderWindow);

    // backface culling
    vtkNew<vtkLightsPass> lightsPass;
    vtkNew<vtkHiddenLineRemovalPass> hlrPass;

    vtkNew<vtkRenderPassCollection> collection;
    collection->AddItem(lightsPass);
    collection->AddItem(hlrPass);

    vtkNew<vtkSequencePass> seqPass;
    seqPass->SetPasses(collection);

    vtkNew<vtkCameraPass> cameraPass;
    cameraPass->SetDelegatePass(seqPass);

    m_rightRenderer->SetPass(cameraPass);
}

void VTKOpenGLWidget::createTestData() {
    // left side
    vtkNew<vtkSTLReader> reader;
    reader->SetFileName("D:/2.stl");
    reader->Update();

    vtkNew<vtkPolyDataMapper> mapperLeft;
    mapperLeft->SetInputConnection(reader->GetOutputPort());

    vtkNew<vtkActor> actorLeft;
    actorLeft->SetMapper(mapperLeft);
    actorLeft->GetProperty()->EdgeVisibilityOn();

    m_leftRenderer->AddActor(actorLeft);

    // Right side
    vtkNew<IsotropicRemeshingFilter> remeshFilter;
    remeshFilter->SetInputConnection(reader->GetOutputPort());
    remeshFilter->SetNumberOfClusters(500);

    vtkNew<vtkPolyDataMapper> mapperRight;
    mapperRight->SetInputConnection(remeshFilter->GetOutputPort());

    vtkNew<vtkActor> actorRight;
    actorRight->SetMapper(mapperRight);
    actorRight->GetProperty()->SetRepresentationToWireframe();
    // Without this line, Sometimes, some edges look like having different
    // color and it depends on edge color and background color.
    actorRight->GetProperty()->LightingOff();
    actorRight->GetProperty()->SetColor(1.0, 1.0, 1.0);

    m_rightRenderer->AddActor(actorRight);
}
