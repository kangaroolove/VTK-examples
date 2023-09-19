#include <QApplication>
#include <QSurfaceFormat>
#include <QVTKOpenGLNativeWidget.h>

int main( int argc, char** argv )
{
  // needed to ensure appropriate OpenGL context is created for VTK rendering.
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

  QApplication app( argc, argv );
  QVTKOpenGLNativeWidget widget;
  widget.show();

  return app.exec();
}
