#pragma once

#include <QMainWindow>

class VTKOpenGLWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRigidRegistration();

private:
    void createMenuBar();

    VTKOpenGLWidget *m_vtkWidget;
};
