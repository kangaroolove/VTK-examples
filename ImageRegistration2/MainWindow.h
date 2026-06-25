#pragma once

#include <QMainWindow>

class QDialog;
class VTKOpenGLWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRigidRegistration();
    void onOpacityControls();

private:
    void createMenuBar();

    VTKOpenGLWidget *m_vtkWidget;
    QDialog *m_opacityDialog = nullptr;
};
