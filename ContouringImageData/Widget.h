#pragma once

#include <QWidget>
class VTKOpenGLWidget;
class QHBoxLayout;
class QPushButton;
class QCheckBox;

class Widget : public QWidget {
  Q_OBJECT
public:
  Widget(QWidget *parent = nullptr);
private slots:
  void onBtnSaveImageClicked();
  void onBtnAutoFillClicked();

private:
  void initGui();
  QHBoxLayout *getToolButtonLayout();
  void bindConnections();

  VTKOpenGLWidget *m_openGLWidget;
  QPushButton *m_btnSaveImage;
  QCheckBox *m_eraseCheckBox;
  QPushButton *m_btnAutoFill;
};