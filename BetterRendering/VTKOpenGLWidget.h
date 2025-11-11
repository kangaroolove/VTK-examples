#pragma once

#include <QElapsedTimer>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class QTimer;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget {
    Q_OBJECT
public:
    VTKOpenGLWidget(QWidget *parent = nullptr);
    ~VTKOpenGLWidget();
private slots:
    void forceRender();
    void scheduleRender();
    void onRenderTimerTimeout();

private:
    void initialize();
    void createTestData();
    void initRenderTimer();

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    QTimer *m_renderTimer;
};