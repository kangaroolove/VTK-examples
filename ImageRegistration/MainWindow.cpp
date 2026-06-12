#include "MainWindow.h"
#include "VTKOpenGLWidget.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSlider>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_vtkWidget(new VTKOpenGLWidget(this)),
      m_opacityDialog(nullptr) {
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

    QMenu *registrationMenu = menuBar()->addMenu(tr("&Registration"));

    QAction *fixedCenterAction = registrationMenu->addAction(
        tr("Set Prostate Center on &Fixed (Ultrasound)"));
    connect(fixedCenterAction, &QAction::triggered, this,
            &MainWindow::setFixedProstateCenter);

    QAction *movingCenterAction = registrationMenu->addAction(
        tr("Set Prostate Center on &Moving (MRI)"));
    connect(movingCenterAction, &QAction::triggered, this,
            &MainWindow::setMovingProstateCenter);

    registrationMenu->addSeparator();

    QAction *rigidAction = registrationMenu->addAction(tr("&Rigid (Ultrasound \xe2\x86\x90 MRI)"));
    rigidAction->setShortcut(Qt::CTRL + Qt::Key_R);
    connect(rigidAction, &QAction::triggered, this, &MainWindow::registerImages);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    QAction *opacityAction = viewMenu->addAction(tr("&Opacity..."));
    connect(opacityAction, &QAction::triggered, this, &MainWindow::showOpacityDialog);

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
    if (!m_vtkWidget->loadImage(fileName, VTKOpenGLWidget::FixedImage, error)) {
        QMessageBox::warning(this, tr("Open Fixed Image"), error);
    }
}

void MainWindow::openMovingImage() {
    const QString fileName = promptForImageFile(tr("Open Moving Image"));
    if (fileName.isEmpty())
        return;

    QString error;
    if (!m_vtkWidget->loadImage(fileName, VTKOpenGLWidget::MovingImage, error)) {
        QMessageBox::warning(this, tr("Open Moving Image"), error);
    }
}

void MainWindow::showOpacityDialog() {
    if (!m_opacityDialog) {
        m_opacityDialog = new QDialog(this);
        m_opacityDialog->setWindowTitle(tr("Image Opacity"));

        QFormLayout *form = new QFormLayout(m_opacityDialog);
        const auto addOpacityRow = [this, form](const QString &label,
                                                VTKOpenGLWidget::ImageRole role) {
            QSlider *slider = new QSlider(Qt::Horizontal);
            slider->setRange(0, 100);
            slider->setValue(qRound(m_vtkWidget->imageOpacity(role) * 100.0));
            slider->setMinimumWidth(180);

            QLabel *value = new QLabel(tr("%1%").arg(slider->value()));
            value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            // Wide enough for "100%" so the slider does not shift as it moves.
            value->setMinimumWidth(value->fontMetrics().horizontalAdvance(
                QStringLiteral("100%")));

            connect(slider, &QSlider::valueChanged, this,
                    [this, role, value](int percent) {
                        value->setText(tr("%1%").arg(percent));
                        m_vtkWidget->setImageOpacity(role, percent / 100.0);
                    });

            QHBoxLayout *row = new QHBoxLayout;
            row->addWidget(slider);
            row->addWidget(value);
            form->addRow(label, row);
        };

        addOpacityRow(tr("Fixed image:"), VTKOpenGLWidget::FixedImage);
        addOpacityRow(tr("Moving image:"), VTKOpenGLWidget::MovingImage);
    }
    m_opacityDialog->show();
    m_opacityDialog->raise();
    m_opacityDialog->activateWindow();
}

void MainWindow::setFixedProstateCenter() {
    if (m_vtkWidget->setProstateCenter(VTKOpenGLWidget::FixedImage)) {
        statusBar()->showMessage(
            tr("Fixed (ultrasound) prostate center set at the crosshair."), 4000);
    } else {
        QMessageBox::warning(
            this, tr("Set Prostate Center"),
            tr("Load the fixed (ultrasound) image first, then place the "
               "crosshair on the prostate."));
    }
}

void MainWindow::setMovingProstateCenter() {
    if (m_vtkWidget->setProstateCenter(VTKOpenGLWidget::MovingImage)) {
        statusBar()->showMessage(
            tr("Moving (MRI) prostate center set at the crosshair."), 4000);
    } else {
        QMessageBox::warning(
            this, tr("Set Prostate Center"),
            tr("Load the moving (MRI) image first, then place the crosshair on "
               "the prostate."));
    }
}

void MainWindow::registerImages() {
    const bool usedLandmarks =
        m_vtkWidget->hasProstateCenter(VTKOpenGLWidget::FixedImage) &&
        m_vtkWidget->hasProstateCenter(VTKOpenGLWidget::MovingImage);

    // Registration may take a moment, so show a wait cursor while it runs.
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString error;
    const bool ok = m_vtkWidget->registerMovingToFixed(error);
    QApplication::restoreOverrideCursor();

    if (ok) {
        const QString detail =
            usedLandmarks
                ? tr("Started from the prostate centers you selected.")
                : tr("No prostate centers were selected, so the volumes' "
                     "geometric centers were aligned first. For a faster and "
                     "more robust result, set a prostate center on each image.");
        QMessageBox::information(
            this, tr("Rigid Registration"),
            tr("Registration finished. The moving image (MRI) has been aligned "
               "to the fixed image (ultrasound).\n\n%1")
                .arg(detail));
    } else {
        QMessageBox::warning(this, tr("Rigid Registration"), error);
    }
}

QString MainWindow::promptForImageFile(const QString &title) {
    return QFileDialog::getOpenFileName(
        this, title, QString(),
        tr("NRRD Images (*.nrrd *.nhdr);;All Files (*)"));
}
