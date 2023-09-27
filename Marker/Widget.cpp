#include "Widget.h"
#include <QVBoxLayout>
#include "VTKOpenGLWidget.h"

Widget::Widget(QWidget* parent)
    : QWidget(parent)
    , m_openGLWidget(new VTKOpenGLWidget(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_openGLWidget);
}

Widget::~Widget()
{

}