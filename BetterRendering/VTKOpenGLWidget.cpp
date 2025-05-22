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
      m_requestTimer(new QTimer(this)),
      m_MaximumUpdateRate(60) {
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::timeout() { forceRender(); }

void VTKOpenGLWidget::initialize() {
    m_requestTimer->setSingleShot(true);
    connect(m_requestTimer, &QTimer::timeout, this, &VTKOpenGLWidget::timeout);

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

void VTKOpenGLWidget::forceRender() {
    if (!m_requestTime.isValid()) return;

    // The timer can be stopped if it hasn't time out yet;
    m_requestTimer->stop();

    m_requestTime.invalidate();

    if (!this->isVisible()) return;

    this->m_renderWindow->Render();
}

void VTKOpenGLWidget::scheduleRender() {
    double msecsBeforeRender = 0;
    // If the MaximumUpdateRate is set to 0 then it indicates that rendering is
    // done next time the application is idle
    if (m_MaximumUpdateRate > 0) {
        msecsBeforeRender = 1000. / m_MaximumUpdateRate;
    } else if (!m_requestTime.isValid()) {
        m_requestTime.start();
        m_requestTimer->start(static_cast<int>(msecsBeforeRender));
    } else if (m_requestTime.elapsed() > msecsBeforeRender) {
        // The rendering hasn't still be done, but msecsBeforeRender
        // milliseconds have already been elapsed, it is likely that
        // RequestTimer has already timed out, but the event queue hasn't been
        // processed yet, rendering is done now to ensure the desired framerate
        // is respected.
        this->forceRender();
    }
}
