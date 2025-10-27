#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QPanGesture>
#include <QSwipeGesture>

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class vtkCamera;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget {
    Q_OBJECT

public:
    VTKOpenGLWidget(QWidget *parent = nullptr);
    ~VTKOpenGLWidget();

protected:
    bool event(QEvent *event) override;
    bool gestureEvent(QGestureEvent *event);
    void panTriggered(QPanGesture *gesture);
    void pinchTriggered(QPinchGesture *gesture);
    void swipeTriggered(QSwipeGesture *gesture);

private:
    void initialize();
    void createTestData();
    void setupGestures();

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkCamera> m_camera;
    
    // Gesture state
    QPointF m_lastPanPoint;
    qreal m_lastScaleFactor;
};