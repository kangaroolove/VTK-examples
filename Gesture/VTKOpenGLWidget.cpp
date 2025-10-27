#include "VTKOpenGLWidget.h"
#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkCamera.h>
#include <QDebug>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_renderer(vtkSmartPointer<vtkRenderer>::New()),
      m_camera(vtkSmartPointer<vtkCamera>::New()),
      m_lastScaleFactor(1.0) {
    initialize();
    createTestData();
    setupGestures();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
    m_renderWindow->AddRenderer(m_renderer);
    m_renderer->SetActiveCamera(m_camera);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData() {
    vtkNew<vtkConeSource> cone;

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(cone->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    m_renderer->AddActor(actor);
    m_renderer->ResetCamera();
}

void VTKOpenGLWidget::setupGestures() {
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::SwipeGesture);
}

bool VTKOpenGLWidget::event(QEvent *event) {
    if (event->type() == QEvent::Gesture) {
        return gestureEvent(static_cast<QGestureEvent*>(event));
    }
    return QVTKOpenGLNativeWidget::event(event);
}

bool VTKOpenGLWidget::gestureEvent(QGestureEvent *event) {
    if (QGesture *pan = event->gesture(Qt::PanGesture)) {
        panTriggered(static_cast<QPanGesture *>(pan));
    }
    if (QGesture *pinch = event->gesture(Qt::PinchGesture)) {
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
    }
    if (QGesture *swipe = event->gesture(Qt::SwipeGesture)) {
        swipeTriggered(static_cast<QSwipeGesture *>(swipe));
    }
    return true;
}

void VTKOpenGLWidget::panTriggered(QPanGesture *gesture) {
    QPointF delta = gesture->delta();
    
    if (gesture->state() == Qt::GestureStarted) {
        m_lastPanPoint = gesture->lastOffset();
    } else if (gesture->state() == Qt::GestureUpdated) {
        // Convert screen coordinates to world coordinates for camera movement
        double deltaX = delta.x() * 0.01;
        double deltaY = delta.y() * 0.01;
        
        // Get current camera position and focal point
        double *position = m_camera->GetPosition();
        double *focalPoint = m_camera->GetFocalPoint();
        double *viewUp = m_camera->GetViewUp();
        
        // Calculate right vector (cross product of view direction and up vector)
        double viewDir[3] = {focalPoint[0] - position[0], focalPoint[1] - position[1], focalPoint[2] - position[2]};
        double rightVector[3];
        rightVector[0] = viewDir[1] * viewUp[2] - viewDir[2] * viewUp[1];
        rightVector[1] = viewDir[2] * viewUp[0] - viewDir[0] * viewUp[2];
        rightVector[2] = viewDir[0] * viewUp[1] - viewDir[1] * viewUp[0];
        
        // Normalize vectors
        double rightLength = sqrt(rightVector[0]*rightVector[0] + rightVector[1]*rightVector[1] + rightVector[2]*rightVector[2]);
        for (int i = 0; i < 3; i++) {
            rightVector[i] /= rightLength;
        }
        
        // Update camera position and focal point
        for (int i = 0; i < 3; i++) {
            position[i] += deltaX * rightVector[i] + deltaY * viewUp[i];
            focalPoint[i] += deltaX * rightVector[i] + deltaY * viewUp[i];
        }
        
        m_camera->SetPosition(position);
        m_camera->SetFocalPoint(focalPoint);
        
        update();
    }
}

void VTKOpenGLWidget::pinchTriggered(QPinchGesture *gesture) {
    QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
    
    if (changeFlags & QPinchGesture::ScaleFactorChanged) {
        qreal currentScaleFactor = gesture->totalScaleFactor();
        
        if (gesture->state() == Qt::GestureStarted) {
            m_lastScaleFactor = currentScaleFactor;
        } else if (gesture->state() == Qt::GestureUpdated) {
            qreal scaleDelta = currentScaleFactor / m_lastScaleFactor;
            
            // Zoom the camera
            double *position = m_camera->GetPosition();
            double *focalPoint = m_camera->GetFocalPoint();
            
            // Calculate direction vector from focal point to camera
            double direction[3];
            for (int i = 0; i < 3; i++) {
                direction[i] = position[i] - focalPoint[i];
            }
            
            // Scale the direction vector (inverse scale for intuitive zoom)
            double zoomFactor = 1.0 / scaleDelta;
            for (int i = 0; i < 3; i++) {
                direction[i] *= zoomFactor;
                position[i] = focalPoint[i] + direction[i];
            }
            
            m_camera->SetPosition(position);
            m_lastScaleFactor = currentScaleFactor;
            
            update();
        }
    }
}

void VTKOpenGLWidget::swipeTriggered(QSwipeGesture *gesture) {
    if (gesture->state() == Qt::GestureFinished) {
        // Rotate camera around the focal point based on swipe direction
        QSwipeGesture::SwipeDirection direction = gesture->horizontalDirection();
        
        double angle = 30.0; // degrees
        if (direction == QSwipeGesture::Left) {
            m_camera->Azimuth(-angle);
        } else if (direction == QSwipeGesture::Right) {
            m_camera->Azimuth(angle);
        }
        
        QSwipeGesture::SwipeDirection verticalDirection = gesture->verticalDirection();
        if (verticalDirection == QSwipeGesture::Up) {
            m_camera->Elevation(angle);
        } else if (verticalDirection == QSwipeGesture::Down) {
            m_camera->Elevation(-angle);
        }
        
        update();
    }
}

