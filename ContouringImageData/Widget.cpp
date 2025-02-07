#include "Widget.h"
#include "VTKOpenGLWidget.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

Widget::Widget(QWidget *parent)
    : QWidget(parent), m_openGLWidget(new VTKOpenGLWidget),
      m_btnSaveImage(new QPushButton("Save Image", this)),
      m_eraseCheckBox(new QCheckBox("erase", this)),
      m_btnAutoFill(new QPushButton("Auto fill", this)) {
  initGui();
  bindConnections();
}

void Widget::onBtnAutoFillClicked() { m_openGLWidget->autoFill(); }

void Widget::initGui() {
  auto layout = new QVBoxLayout(this);
  layout->addWidget(m_openGLWidget);
  layout->addLayout(getToolButtonLayout());
  this->resize(800, 600);
}

QHBoxLayout *Widget::getToolButtonLayout() {
  auto layout = new QHBoxLayout();
  layout->addWidget(m_btnAutoFill);
  layout->addWidget(m_btnSaveImage);
  layout->addWidget(m_eraseCheckBox);
  return layout;
}

void Widget::bindConnections() {
  connect(m_btnSaveImage, &QPushButton::clicked, this,
          &Widget::onBtnSaveImageClicked);
  connect(m_eraseCheckBox, &QCheckBox::toggled, this,
          [this](bool checked) { m_openGLWidget->setEraseOn(checked); });
  connect(m_btnAutoFill, &QPushButton::clicked, this,
          &Widget::onBtnAutoFillClicked);
}

void Widget::onBtnSaveImageClicked() { m_openGLWidget->saveImageToLocal(); }
