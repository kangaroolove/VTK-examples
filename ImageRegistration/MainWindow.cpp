#include "MainWindow.h"
#include "VTKOpenGLWidget.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_vtkWidget(new VTKOpenGLWidget(this)) {
    setWindowTitle(tr("Image Registration"));
    setCentralWidget(m_vtkWidget);
    createMenus();
    resize(800, 800);
}

void MainWindow::createMenus() {
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *openAction = fileMenu->addAction(tr("&Open Image..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openImage);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction *aboutAction = helpMenu->addAction(tr("&About"));
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("About Image Registration"),
                           tr("Image registration example with VTK and Qt."));
    });
}

void MainWindow::openImage() {
    const QString fileName = QFileDialog::getOpenFileName(
        this, tr("Open Image"), QString(),
        tr("Images (*.mhd *.mha *.nii *.nii.gz *.dcm);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    // TODO: load the selected image into the slice views.
    QMessageBox::information(this, tr("Open Image"),
                             tr("Image loading is not implemented yet:\n%1").arg(fileName));
}
