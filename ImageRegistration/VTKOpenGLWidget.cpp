#include "VTKOpenGLWidget.h"
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkImageEllipsoidSource.h>
#include <vtkImageProperty.h>
#include <vtkImageSlice.h>
#include <vtkImageSliceMapper.h>
#include <vtkInteractorStyleImage.h>
#include <vtkNew.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_transverseRenderer(vtkSmartPointer<vtkRenderer>::New()),
      m_sagittalRenderer(vtkSmartPointer<vtkRenderer>::New()),
      m_coronalRenderer(vtkSmartPointer<vtkRenderer>::New()) {
    initialize();
    createTestData();
}

void VTKOpenGLWidget::initialize() {
    // 2x2 layout: transverse top-left, sagittal top-right, coronal bottom-left
    m_transverseRenderer->SetViewport(0.0, 0.5, 0.5, 1.0);
    m_sagittalRenderer->SetViewport(0.5, 0.5, 1.0, 1.0);
    m_coronalRenderer->SetViewport(0.0, 0.0, 0.5, 0.5);

    m_transverseRenderer->SetBackground(0.0, 0.0, 0.0);
    m_sagittalRenderer->SetBackground(0.05, 0.05, 0.05);
    m_coronalRenderer->SetBackground(0.1, 0.1, 0.1);

    m_renderWindow->AddRenderer(m_transverseRenderer);
    m_renderWindow->AddRenderer(m_sagittalRenderer);
    m_renderWindow->AddRenderer(m_coronalRenderer);
    SetRenderWindow(m_renderWindow);

    vtkNew<vtkInteractorStyleImage> style;
    GetInteractor()->SetInteractorStyle(style);
}

void VTKOpenGLWidget::createTestData() {
    // Placeholder volume until a real medical image is loaded.
    // Different radii per axis so the three views look different.
    vtkNew<vtkImageEllipsoidSource> ellipsoid;
    ellipsoid->SetWholeExtent(0, 99, 0, 99, 0, 99);
    ellipsoid->SetCenter(50.0, 50.0, 50.0);
    ellipsoid->SetRadius(40.0, 25.0, 15.0);
    ellipsoid->SetInValue(255.0);
    ellipsoid->SetOutValue(0.0);
    ellipsoid->Update();

    vtkImageData *image = ellipsoid->GetOutput();
    setupSliceView(m_transverseRenderer, image, Transverse);
    setupSliceView(m_sagittalRenderer, image, Sagittal);
    setupSliceView(m_coronalRenderer, image, Coronal);
}

void VTKOpenGLWidget::setupSliceView(vtkRenderer *renderer, vtkImageData *image,
                                     int orientation) {
    vtkNew<vtkImageSliceMapper> mapper;
    mapper->SetInputData(image);
    mapper->SetOrientation(orientation);
    mapper->SetSliceNumber(
        (mapper->GetSliceNumberMinValue() + mapper->GetSliceNumberMaxValue()) / 2);

    vtkNew<vtkImageSlice> slice;
    slice->SetMapper(mapper);
    slice->GetProperty()->SetColorWindow(255.0);
    slice->GetProperty()->SetColorLevel(127.5);

    renderer->AddViewProp(slice);

    double center[3];
    image->GetCenter(center);

    vtkCamera *camera = renderer->GetActiveCamera();
    camera->ParallelProjectionOn();
    camera->SetFocalPoint(center);
    switch (orientation) {
    case Sagittal: // look along -X
        camera->SetPosition(center[0] + 500.0, center[1], center[2]);
        camera->SetViewUp(0.0, 0.0, 1.0);
        break;
    case Coronal: // look along +Y
        camera->SetPosition(center[0], center[1] - 500.0, center[2]);
        camera->SetViewUp(0.0, 0.0, 1.0);
        break;
    case Transverse: // look along -Z
        camera->SetPosition(center[0], center[1], center[2] + 500.0);
        camera->SetViewUp(0.0, 1.0, 0.0);
        break;
    }
    renderer->ResetCamera();
}
