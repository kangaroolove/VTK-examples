#include <QApplication>
#include <QSurfaceFormat>
#include "VTKOpenGLWidget.h"

int main( int argc, char** argv )
{
  // needed to ensure appropriate OpenGL context is created for VTK rendering.
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

  QApplication app( argc, argv );
  VTKOpenGLWidget widget;
  widget.showMaximized();

  return app.exec();
}
