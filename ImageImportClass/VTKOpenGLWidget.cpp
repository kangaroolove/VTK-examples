#include "VTKOpenGLWidget.h"
#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <QImage>
#include <vtkImageImport.h>
#include <vtkImageActor.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_renderer(vtkSmartPointer<vtkRenderer>::New()),
      m_image(QImage("D:/1.png")) {
    initialize();
    createTestData();
}

VTKOpenGLWidget::~VTKOpenGLWidget() {}

void VTKOpenGLWidget::initialize() {
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
}

void VTKOpenGLWidget::createTestData() {
    // In order to convert to an 8 X 8 X 8 array, This step is must-have
    m_image = m_image.convertToFormat(QImage::Format_RGB888);

    int width  = m_image.width();
    int height = m_image.height();
    // 3 for RGB888, 4 for RGBA8888
    int components = m_image.depth() / 8; 

    auto data = m_image.bits();

    vtkNew<vtkImageImport> importer;

    // This setting is important
    importer->SetDataScalarTypeToUnsignedChar();
    
    importer->SetNumberOfScalarComponents(components);
    // Very important: tell VTK the extent (whole image)
    importer->SetDataExtent(0, width - 1, 0, height - 1, 0, 0);
    importer->SetWholeExtent(0, width - 1, 0, height - 1, 0, 0);

    // Pixel spacing (1.0, 1.0) and origin (0,0) is usually fine
    importer->SetDataSpacing(1.0, 1.0, 1.0);
    importer->SetDataOrigin(0.0, 0.0, 0.0);
    importer->SetImportVoidPointer(data);

    importer->Update();

    vtkNew<vtkImageActor> actor;
    actor->SetInputData(importer->GetOutput());

    m_renderer->AddActor(actor);
}
