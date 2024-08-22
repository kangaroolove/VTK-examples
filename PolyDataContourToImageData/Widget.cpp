#include "Widget.h"
#include "VTKOpenGLWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>

Widget::Widget(QWidget* parent) :
    QWidget(parent),
    m_openGLWidget(new VTKOpenGLWidget),
    m_btnSaveImage(new QPushButton("Save Image", this))
{
    initGui();
    bindConnections();
}

void Widget::initGui()
{
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_openGLWidget);
    layout->addLayout(getToolButtonLayout());
    this->resize(800, 600);
}

QHBoxLayout *Widget::getToolButtonLayout()
{
    auto layout = new QHBoxLayout();
    layout->addWidget(m_btnSaveImage);
    return layout;
}

void Widget::bindConnections()
{
    connect(m_btnSaveImage, &QPushButton::clicked, this, &Widget::onBtnSaveImageClicked);
}

void Widget::onBtnSaveImageClicked()
{
    m_openGLWidget->saveImageToLocal();
}
