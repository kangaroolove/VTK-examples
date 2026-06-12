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

    QAction *openFixedAction = fileMenu->addAction(tr("Open &Fixed Image..."));
    openFixedAction->setShortcut(QKeySequence::Open);
    connect(openFixedAction, &QAction::triggered, this, &MainWindow::openFixedImage);

    QAction *openMovingAction = fileMenu->addAction(tr("Open &Moving Image..."));
    connect(openMovingAction, &QAction::triggered, this, &MainWindow::openMovingImage);

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

void MainWindow::openFixedImage() {
    const QString fileName = promptForImageFile(tr("Open Fixed Image"));
    if (fileName.isEmpty())
        return;

    // TODO: load the selected fixed image into the slice views.
    QMessageBox::information(this, tr("Open Fixed Image"),
                             tr("Image loading is not implemented yet:\n%1").arg(fileName));
}

void MainWindow::openMovingImage() {
    const QString fileName = promptForImageFile(tr("Open Moving Image"));
    if (fileName.isEmpty())
        return;

    // TODO: load the selected moving image into the slice views.
    QMessageBox::information(this, tr("Open Moving Image"),
                             tr("Image loading is not implemented yet:\n%1").arg(fileName));
}

QString MainWindow::promptForImageFile(const QString &title) {
    return QFileDialog::getOpenFileName(
        this, title, QString(),
        tr("Images (*.mhd *.mha *.nii *.nii.gz *.dcm);;All Files (*)"));
}
