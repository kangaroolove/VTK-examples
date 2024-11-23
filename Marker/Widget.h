#pragma once

#include <QWidget>

class VTKOpenGLWidget;

class Widget : public QWidget {
public:
  Widget(QWidget *parent = nullptr);
  ~Widget();

private:
  VTKOpenGLWidget *m_openGLWidget;
};