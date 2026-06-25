#include "MainWindow.h"
#include "VTKOpenGLWidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_vtkWidget(new VTKOpenGLWidget(this))
{
    setCentralWidget(m_vtkWidget);
    setWindowTitle("Image Registration");
    resize(800, 600);
}

MainWindow::~MainWindow() {}
