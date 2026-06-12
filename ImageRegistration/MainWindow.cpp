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

    QString error;
    if (!m_vtkWidget->loadImage(fileName, error)) {
        QMessageBox::warning(this, tr("Open Fixed Image"), error);
    }
}

void MainWindow::openMovingImage() {
    const QString fileName = promptForImageFile(tr("Open Moving Image"));
    if (fileName.isEmpty())
        return;

    // TODO: overlay the moving image on the fixed one once registration is
    // implemented; for now it simply replaces the displayed volume.
    QString error;
    if (!m_vtkWidget->loadImage(fileName, error)) {
        QMessageBox::warning(this, tr("Open Moving Image"), error);
    }
}

QString MainWindow::promptForImageFile(const QString &title) {
    return QFileDialog::getOpenFileName(
        this, title, QString(),
        tr("NRRD Images (*.nrrd *.nhdr);;All Files (*)"));
}
