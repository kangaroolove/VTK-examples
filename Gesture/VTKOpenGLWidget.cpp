#include "VTKOpenGLWidget.h"

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkConeSource.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include <QDebug>

class KeyPressInteractorStyle : public vtkInteractorStyleTrackballCamera {
public:
    static KeyPressInteractorStyle *New();
    vtkTypeMacro(KeyPressInteractorStyle, vtkInteractorStyleTrackballCamera);

    void OnMouseMove() override {
        if (m_pinching) {
            // double dyf =
            //     this->Interactor->GetScale() /
            //     this->Interactor->GetLastScale();

            // auto dyf = 0.1;
            // // qDebug() << "dyf = " << dyf;

            // auto camera = mRenderer->GetActiveCamera();
            // camera->Dolly(dyf);
            // if (this->AutoAdjustCameraClippingRange) {
            //     this->mRenderer->ResetCameraClippingRange();
            // }

            // mRenderWindow->Render();
            return;
        }

        // qDebug() << "VTK OnMouseMove";

        vtkInteractorStyleTrackballCamera::OnMouseMove();
        // qDebug()<<"OnMouseMove";
    }

    // virtual void OnKeyPress() override {
    //     vtkRenderWindowInteractor *rwi = this->Interactor;
    //     std::string key = rwi->GetKeySym();

    //     std::cout << "Pressed " << key << endl;

    //     vtkNew<vtkTransform> transform;
    //     vtkLinearTransform *userTransform =
    //     mConeActor->GetUserTransform(); if (key == "Up") {
    //         std::cout<< *mConeActor->GetMatrix()<<endl;

    //         auto userMatrix = mConeActor->GetUserMatrix();
    //         if (userMatrix)
    //             std::cout<<*userMatrix<<endl;
    //         // transform->SetInput(userTransform);
    //         // transform->Translate(0, 1, 0);
    //         // mConeActor->SetUserTransform(transform);
    //     } else if (key == "Down") {
    //         transform->SetInput(userTransform);
    //         transform->Translate(0, -1, 0);
    //         mConeActor->SetUserTransform(transform);
    //     } else if (key == "Left") {
    //         transform->SetInput(userTransform);
    //         transform->Translate(-1, 0, 0);
    //         mConeActor->SetUserTransform(transform);
    //     } else if (key == "Right") {

    //     }
    //     mRenderWindow->Render();

    //     vtkInteractorStyleTrackballCamera::OnKeyPress();
    // }
    void SetConeActor(vtkActor *coneActor) { mConeActor = coneActor; }
    void SetRenderWindow(vtkRenderWindow *window) { mRenderWindow = window; }
    void SetPinching(bool pinching) { m_pinching = pinching; }
    void SetRenderer(vtkRenderer *renderer) { mRenderer = renderer; }

private:
    vtkActor *mConeActor;
    vtkRenderWindow *mRenderWindow;
    vtkRenderer *mRenderer;
    bool m_pinching = false;
};
vtkStandardNewMacro(KeyPressInteractorStyle);

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_renderer(vtkSmartPointer<vtkRenderer>::New()),
      m_camera(vtkSmartPointer<vtkCamera>::New()),
      m_lastScaleFactor(1.0),
      m_style(vtkSmartPointer<KeyPressInteractorStyle>::New()) {
    initialize();
    createTestData();
    setupGestures();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
    m_renderWindow->AddRenderer(m_renderer);
    m_renderer->SetActiveCamera(m_camera);
    SetRenderWindow(m_renderWindow);

    auto it = m_renderWindow->GetInteractor();
    m_style->SetRenderWindow(m_renderWindow);
    m_style->SetRenderer(m_renderer);
    it->SetInteractorStyle(m_style);
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
    // qDebug() << "Qt Event = " << event;
    return QVTKOpenGLNativeWidget::event(event);
}

bool VTKOpenGLWidget::gestureEvent(QGestureEvent *event) {
    // if (QGesture *pan = event->gesture(Qt::PanGesture)) {
    //     panTriggered(static_cast<QPanGesture *>(pan));
    // }
    if (QGesture *pinch = event->gesture(Qt::PinchGesture)) {
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
    }
    // if (QGesture *swipe = event->gesture(Qt::SwipeGesture)) {
    //     swipeTriggered(static_cast<QSwipeGesture *>(swipe));
    // }
    return true;
}

void VTKOpenGLWidget::panTriggered(QPanGesture *gesture) {
    QPointF delta = gesture->delta();

    if (gesture->state() == Qt::GestureStarted) {
        m_lastPanPoint = gesture->lastOffset();
    } else if (gesture->state() == Qt::GestureUpdated) {
        // Convert screen coordinates to world coordinates for camera
        // movement
        double deltaX = delta.x() * 0.01;
        double deltaY = delta.y() * 0.01;

        // Get current camera position and focal point
        double *position = m_camera->GetPosition();
        double *focalPoint = m_camera->GetFocalPoint();
        double *viewUp = m_camera->GetViewUp();

        // Calculate right vector (cross product of view direction and up
        // vector)
        double viewDir[3] = {focalPoint[0] - position[0],
                             focalPoint[1] - position[1],
                             focalPoint[2] - position[2]};
        double rightVector[3];
        rightVector[0] = viewDir[1] * viewUp[2] - viewDir[2] * viewUp[1];
        rightVector[1] = viewDir[2] * viewUp[0] - viewDir[0] * viewUp[2];
        rightVector[2] = viewDir[0] * viewUp[1] - viewDir[1] * viewUp[0];

        // Normalize vectors
        double rightLength = sqrt(rightVector[0] * rightVector[0] +
                                  rightVector[1] * rightVector[1] +
                                  rightVector[2] * rightVector[2]);
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
    if (gesture->state() == Qt::GestureFinished) {
        qDebug() << "Pinch finished";
        m_style->SetPinching(false);
    }

    if (gesture->state() == Qt::GestureStarted) {
        qDebug() << "Pinch started";
        qreal currentScaleFactor = gesture->totalScaleFactor();
        qDebug() << "currentScaleFactor = " << currentScaleFactor;
        m_style->SetPinching(true);
        m_lastScaleFactor = currentScaleFactor;
    }

    if (changeFlags & QPinchGesture::ScaleFactorChanged) {
        qreal currentScaleFactor = gesture->totalScaleFactor();
        if (gesture->state() == Qt::GestureUpdated) {
            qreal scaleDelta = currentScaleFactor / m_lastScaleFactor;

            qDebug() << "currentScaleFactor = " << currentScaleFactor;
            qDebug() << "m_lastScaleFactor = " << m_lastScaleFactor;
            qDebug() << "scaleDelta = " << scaleDelta;

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

            m_renderer->ResetCameraClippingRange();

            // update();
            m_renderWindow->Render();
        }
    }
}

void VTKOpenGLWidget::swipeTriggered(QSwipeGesture *gesture) {
    if (gesture->state() == Qt::GestureFinished) {
        // Rotate camera around the focal point based on swipe direction
        QSwipeGesture::SwipeDirection direction =
            gesture->horizontalDirection();

        double angle = 30.0;  // degrees
        if (direction == QSwipeGesture::Left) {
            m_camera->Azimuth(-angle);
        } else if (direction == QSwipeGesture::Right) {
            m_camera->Azimuth(angle);
        }

        QSwipeGesture::SwipeDirection verticalDirection =
            gesture->verticalDirection();
        if (verticalDirection == QSwipeGesture::Up) {
            m_camera->Elevation(angle);
        } else if (verticalDirection == QSwipeGesture::Down) {
            m_camera->Elevation(-angle);
        }

        update();
    }
}
