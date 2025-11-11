#include "VTKOpenGLWidget.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTime>
#include <QTimer>
#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_renderer(vtkSmartPointer<vtkRenderer>::New()),
      m_renderTimer(new QTimer(this)) {
    initialize();
    initRenderTimer();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData() {
    vtkNew<vtkConeSource> cone;

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(cone->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    m_renderer->AddActor(actor);
}

void VTKOpenGLWidget::initRenderTimer() {
    m_renderTimer->setSingleShot(true);
    const int fps = 60;
    m_renderTimer->setInterval(1000 / fps);

    connect(m_renderTimer, &QTimer::timeout, this,
            &VTKOpenGLWidget::onRenderTimerTimeout);
}

void VTKOpenGLWidget::forceRender() {
    if (m_renderTimer->isActive()) m_renderTimer->stop();
    m_renderWindow->Render();
}

void VTKOpenGLWidget::scheduleRender() {
    if (m_renderTimer->isActive()) return;

    m_renderTimer->start();
}

void VTKOpenGLWidget::onRenderTimerTimeout() { m_renderWindow->Render(); }
