#pragma once

#include <QString>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>

#include <itkImage.h>

class QMouseEvent;
class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class vtkImageActor;
class vtkImageData;
class vtkImageReslice;
class vtkLineSource;
class vtkMatrix4x4;

class VTKOpenGLWidget : public QVTKOpenGLNativeWidget {
public:
    enum SliceOrientation {
        Sagittal = 0,   // YZ plane, slice along X
        Coronal = 1,    // XZ plane, slice along Y
        Transverse = 2  // XY plane, slice along Z
    };

    enum ImageRole {
        FixedImage = 0,
        MovingImage = 1
    };

    VTKOpenGLWidget(QWidget *parent = nullptr);

    // Loads a NRRD image into the given layer and shows it in the three slice
    // views; the moving image is blended over the fixed one. The volume is
    // displayed in world coordinates (always LPS, as read by ITK), including
    // any rotation stored in the file's direction cosines.
    // Returns false and fills errorMessage on failure.
    bool loadImage(const QString &fileName, ImageRole role, QString &errorMessage);

    // Layer opacity in [0, 1].
    void setImageOpacity(ImageRole role, double opacity);
    double imageOpacity(ImageRole role) const;

    // Crosshair position in world (LPS) coordinates, shared by the three views.
    void setCrosshairPosition(double x, double y, double z);

    // Records the current crosshair position as the registration landmark (the
    // prostate center) for the given image. The user positions the crosshair on
    // the prostate in that image -- adjusting opacity to see it -- and calls
    // this. The fixed and moving landmarks initialize the registration so it
    // starts already aligned at the prostate, which is fast and robust even when
    // the two volumes are far apart. Returns false if the image is not loaded.
    bool setProstateCenter(ImageRole role);
    bool setProstateCenter(ImageRole role, int i, int j, int k);
    bool hasProstateCenter(ImageRole role) const;
    bool getImageDimensions(ImageRole role, int dims[3]) const;

    // Rigidly registers the moving image (MRI) onto the fixed image (ultrasound)
    // and repositions the moving layer so the two volumes overlay in world
    // space. If a prostate center has been set on both images, the registration
    // is initialized from those landmarks; otherwise it falls back to aligning
    // the volumes' geometric centers. The registration is fully deterministic:
    // it runs with a fixed work-unit count and a fixed sampling pattern, so the
    // same inputs and landmarks always yield the same result. Both images must
    // be loaded. Returns false and fills errorMessage on failure.
    bool registerMovingToFixed(QString &errorMessage);

protected:
    bool event(QEvent *event) override;

private:
    struct ImageLayer {
        vtkSmartPointer<vtkImageData> image;
        vtkSmartPointer<vtkMatrix4x4> imageToWorld; // image data coords -> LPS world
        // Oriented ITK image in LPS physical space, kept for registration.
        itk::Image<float, 3>::Pointer itkImage;
    };

    struct SliceView {
        vtkSmartPointer<vtkRenderer> renderer;
        vtkSmartPointer<vtkImageReslice> reslices[2]; // indexed by ImageRole
        vtkSmartPointer<vtkImageActor> actors[2];     // indexed by ImageRole
        vtkSmartPointer<vtkLineSource> horizontalLine;
        vtkSmartPointer<vtkLineSource> verticalLine;
        double bounds[6]; // bounds of the resliced images in view coordinates
        int orientation;
    };

    void initialize();
    void createTestData();
    void setImage(ImageRole role, vtkImageData *image, vtkMatrix4x4 *imageToWorld);
    bool hasImage() const;
    void updateWorldBounds();
    void setupSliceView(SliceView &view);
    void updateResliceOrigin(SliceView &view);
    void updateCrosshairLines(SliceView &view);
    void crosshairToViewCoords(const SliceView &view, double &x, double &y) const;

    bool handleCrosshairMouseEvent(QMouseEvent *event);
    void toVtkDisplayCoords(QMouseEvent *event, int &x, int &y) const;
    void moveCrosshairToDisplayPoint(int x, int y);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    ImageLayer m_layers[2];  // indexed by ImageRole
    double m_opacities[2];   // indexed by ImageRole
    double m_worldBounds[6]; // axis-aligned bounds of the volumes in world coords
    SliceView m_views[3]; // indexed by SliceOrientation
    double m_crosshair[3];
    int m_activeViewIndex; // view being dragged, -1 when none

    // Registration landmark (prostate center) per image, in that image's own
    // physical coordinates, plus whether each has been set. Indexed by ImageRole.
    double m_centers[2][3];
    bool m_hasCenter[2];
};
