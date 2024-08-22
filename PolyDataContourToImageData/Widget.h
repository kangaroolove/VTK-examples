#pragma once

#include <QWidget>
class VTKOpenGLWidget;
class QHBoxLayout;
class QPushButton;

class Widget : public QWidget
{
    Q_OBJECT
public:
    Widget(QWidget* parent = nullptr);
private slots:
    void onBtnSaveImageClicked();
private:
    void initGui();
    QHBoxLayout* getToolButtonLayout();
    void bindConnections();

    VTKOpenGLWidget* m_openGLWidget;
    QPushButton* m_btnSaveImage;
    QPushButton* m_btnLoadImage;
};