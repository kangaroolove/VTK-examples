#pragma once

#include <QMainWindow>

class QDialog;
class VTKOpenGLWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    void createMenus();
    void openFixedImage();
    void openMovingImage();
    void showOpacityDialog();
    void setFixedProstateCenter();
    void setMovingProstateCenter();
    void registerImages();
    QString promptForImageFile(const QString &title);

    VTKOpenGLWidget *m_vtkWidget;
    QDialog *m_opacityDialog;
};
