#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>

class QMouseEvent;
class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class vtkImageData;
class vtkImageReslice;
class vtkLineSource;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget {
public:
    enum SliceOrientation {
        Sagittal = 0,   // YZ plane, slice along X
        Coronal = 1,    // XZ plane, slice along Y
        Transverse = 2  // XY plane, slice along Z
    };

    VTKOpenGLWidget(QWidget *parent = nullptr);

    // Crosshair position in image/world coordinates, shared by the three views.
    void setCrosshairPosition(double x, double y, double z);

protected:
    bool event(QEvent *event) override;

private:
    struct SliceView {
        vtkSmartPointer<vtkRenderer> renderer;
        vtkSmartPointer<vtkImageReslice> reslice;
        vtkSmartPointer<vtkLineSource> horizontalLine;
        vtkSmartPointer<vtkLineSource> verticalLine;
        double bounds[6]; // bounds of the resliced image in view coordinates
        int orientation;
    };

    void initialize();
    void createTestData();
    void setupSliceView(SliceView &view, vtkImageData *image);
    void updateResliceOrigin(SliceView &view);
    void updateCrosshairLines(SliceView &view);
    void crosshairToViewCoords(const SliceView &view, double &x, double &y) const;

    bool handleCrosshairMouseEvent(QMouseEvent *event);
    void toVtkDisplayCoords(QMouseEvent *event, int &x, int &y) const;
    void moveCrosshairToDisplayPoint(int x, int y);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkImageData> m_image;
    SliceView m_views[3]; // indexed by SliceOrientation
    double m_crosshair[3];
    int m_activeViewIndex; // view being dragged, -1 when none
};
