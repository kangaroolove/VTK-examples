#include "MainWindow.h"

#include <QAction>
#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QSlider>
#include <QVBoxLayout>

#include "VTKOpenGLWidget.h"

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

    QMenu *viewMenu = menuBar()->addMenu("View");

    QAction *opacityAction = viewMenu->addAction("Opacity Controls...");
    connect(opacityAction, &QAction::triggered, this,
            &MainWindow::onOpacityControls);
}

void MainWindow::onRigidRegistration() {
    m_vtkWidget->runRegistration();
}

void MainWindow::onOpacityControls() {
    if (!m_opacityDialog) {
        m_opacityDialog = new QDialog(this);
        m_opacityDialog->setWindowTitle("Opacity Controls");

        QFormLayout *layout = new QFormLayout(m_opacityDialog);

        QSlider *usSlider = new QSlider(Qt::Horizontal, m_opacityDialog);
        usSlider->setRange(0, 100);
        usSlider->setValue(100);

        QSlider *mriSlider = new QSlider(Qt::Horizontal, m_opacityDialog);
        mriSlider->setRange(0, 100);
        mriSlider->setValue(100);

        layout->addRow("US Opacity:", usSlider);
        layout->addRow("MRI Opacity:", mriSlider);

        connect(usSlider, &QSlider::valueChanged, this,
                [this](int value) { m_vtkWidget->setUsOpacity(value / 100.0); });
        connect(mriSlider, &QSlider::valueChanged, this,
                [this](int value) { m_vtkWidget->setMriOpacity(value / 100.0); });
    }

    m_opacityDialog->show();
    m_opacityDialog->raise();
    m_opacityDialog->activateWindow();
}
