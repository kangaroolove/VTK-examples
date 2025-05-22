#include "VTKOpenGLWidget.h"
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkConeSource.h>
#include <vtkCornerAnnotation.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>
#include <vtkNew.h>
#include <vtkNrrdReader.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()) {
    initialize();
    createTestData();
    createAnnotation();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
    for (auto &renderer : m_renderers) {
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

void VTKOpenGLWidget::createTestData() {
    std::string fileName = "D:/MRI.nrrd";
    vtkNew<vtkNrrdReader> reader;
    reader->SetFileName(fileName.data());
    reader->Update();

    int extent[6] = {0};
    reader->GetDataExtent(extent);

    int middleExtentX = (extent[0] + extent[1]) / 2;
    int middleExtentY = (extent[2] + extent[3]) / 2;
    int middleExtentZ = (extent[4] + extent[5]) / 2;

    std::array<vtkSmartPointer<vtkImageActor>, 3> actors;
    for (auto &actor : actors) {
        actor = vtkSmartPointer<vtkImageActor>::New();
        actor->SetInputData(reader->GetOutput());
    }

actors[0]->SetDisplayExtent(middleExtentX, middleExtentX, extent[2], extent[3],
                            extent[4], extent[5]);
actors[1]->SetDisplayExtent(extent[0], extent[1], middleExtentY, middleExtentY,
                            extent[4], extent[5]);
actors[2]->SetDisplayExtent(extent[0], extent[1], extent[2], extent[3],
                            middleExtentZ, middleExtentZ);

for (int i = 0; i < 3; i++) m_renderers[i]->AddViewProp(actors[i]);

m_renderers.at(2)->GetActiveCamera()->Roll(180);
m_renderers.at(2)->GetActiveCamera()->Azimuth(180);
m_renderers.at(2)->ResetCamera();

m_renderers.at(1)->GetActiveCamera()->Pitch(90);
m_renderers.at(1)->ResetCamera();

m_renderers.at(0)->GetActiveCamera()->Yaw(90);
m_renderers.at(0)->GetActiveCamera()->Roll(-90);
m_renderers.at(0)->ResetCamera();
}

void VTKOpenGLWidget::createAnnotation() {
    vtkNew<vtkCornerAnnotation> rightAnnotation;
    rightAnnotation->SetText(vtkCornerAnnotation::TextPosition::LeftEdge, "R");
    rightAnnotation->SetText(vtkCornerAnnotation::TextPosition::RightEdge, "L");
    rightAnnotation->SetText(vtkCornerAnnotation::TextPosition::UpperEdge, "A");
    rightAnnotation->SetText(vtkCornerAnnotation::TextPosition::LowerEdge, "P");
    m_renderers.at(2)->AddViewProp(rightAnnotation);

    vtkNew<vtkCornerAnnotation> middleAnnotation;
    middleAnnotation->SetText(vtkCornerAnnotation::TextPosition::LeftEdge, "R");
    middleAnnotation->SetText(vtkCornerAnnotation::TextPosition::RightEdge,
                              "L");
    middleAnnotation->SetText(vtkCornerAnnotation::TextPosition::UpperEdge,
                              "S");
    middleAnnotation->SetText(vtkCornerAnnotation::TextPosition::LowerEdge,
                              "I");
    m_renderers.at(1)->AddViewProp(middleAnnotation);

    vtkNew<vtkCornerAnnotation> leftAnnotation;
    leftAnnotation->SetText(vtkCornerAnnotation::TextPosition::LeftEdge, "A");
    leftAnnotation->SetText(vtkCornerAnnotation::TextPosition::RightEdge, "P");
    leftAnnotation->SetText(vtkCornerAnnotation::TextPosition::UpperEdge, "S");
    leftAnnotation->SetText(vtkCornerAnnotation::TextPosition::LowerEdge, "I");
    m_renderers.at(0)->AddViewProp(leftAnnotation);
}
