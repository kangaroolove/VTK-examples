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
  void timeout();
  void forceRender();
  void scheduleRender();

private:
  void initialize();
  void createTestData();

  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
  vtkSmartPointer<vtkRenderer> m_renderer;
  QTimer *m_requestTimer;
  QElapsedTimer m_requestTime;
  double m_MaximumUpdateRate;
};