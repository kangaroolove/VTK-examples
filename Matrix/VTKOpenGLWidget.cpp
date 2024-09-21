#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkTransform.h>
#include <QDebug>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_renderer(vtkSmartPointer<vtkRenderer>::New())
{
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget()
{

}

void VTKOpenGLWidget::initialize()
{
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData()
{
    qDebug()<<"Matrix";
    vtkNew<vtkTransform> transformA;
    transformA->Translate(10, 0, 0);
    std::cout<<"matrixA="<<std::endl<<*transformA->GetMatrix()<<std::endl;

    vtkNew<vtkTransform> transformB;
    transformB->RotateZ(90);
    std::cout<<"matrixB="<<std::endl<<*transformB->GetMatrix()<<std::endl;

    vtkNew<vtkMatrix4x4> matrixC;
    vtkNew<vtkMatrix4x4> matrixD;

    vtkMatrix4x4::Multiply4x4(transformA->GetMatrix(), transformB->GetMatrix(), matrixC);
    vtkMatrix4x4::Multiply4x4(transformB->GetMatrix(), transformA->GetMatrix(), matrixD);

    std::cout<<"C = A x B"<<std::endl<<*matrixC<<std::endl;
    std::cout<<"D = B x A"<<std::endl<<*matrixD<<std::endl;


    qDebug()<<"pre Multiple";
    vtkNew<vtkTransform> transformE;
    transformE->Translate(10, 0, 0);
    transformE->RotateZ(90);
    std::cout<<"transformE="<<std::endl<<*transformE->GetMatrix()<<std::endl;


    qDebug()<<"Post multiple";
    vtkNew<vtkTransform> transformF;
    transformF->PostMultiply();
    transformF->Translate(10, 0, 0);
    transformF->RotateZ(90);
    std::cout<<"transformF="<<std::endl<<*transformF->GetMatrix()<<std::endl;
    

    qDebug()<<"Concatenate";
    vtkNew<vtkTransform> transformG;
    transformG->Concatenate(transformA);
    transformG->Concatenate(transformB);
    std::cout<<"transformG="<<std::endl<<*transformG->GetMatrix()<<std::endl;
}
