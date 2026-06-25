#include "MainWindow.h"
#include "VTKOpenGLWidget.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_vtkWidget(new VTKOpenGLWidget(this))
{
    setCentralWidget(m_vtkWidget);
    setWindowTitle("Image Registration");
    resize(800, 600);
    createMenuBar();
}

MainWindow::~MainWindow() {}

void MainWindow::createMenuBar() {
    QMenu *registrationMenu = menuBar()->addMenu("Registration");

    QAction *rigidAction = registrationMenu->addAction("Rigid Registration (MRI->US)");
    connect(rigidAction, &QAction::triggered, this, &MainWindow::onRigidRegistration);
}

void MainWindow::onRigidRegistration() {
    // TODO: implement rigid registration
    QMessageBox::information(this, "Rigid Registration", "Rigid Registration (MRI->US) triggered.");
}
