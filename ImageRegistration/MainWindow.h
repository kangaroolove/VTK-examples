#pragma once

#include <QMainWindow>

class VTKOpenGLWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    void createMenus();
    void openImage();

    VTKOpenGLWidget *m_vtkWidget;
};
