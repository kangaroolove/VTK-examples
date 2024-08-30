#include "VTKOpenGLWidget.h"
#include <QDebug>
#include <array>
#include <vtkAbstractPicker.h>
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkAppendPolyData.h>
#include <vtkAssemblyPath.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellPicker.h>
#include <vtkCleanPolyData.h>
#include <vtkConeSource.h>
#include <vtkConvexHull2D.h>
#include <vtkCoordinate.h>
#include <vtkCutter.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGlyph3D.h>
#include <vtkImageActor.h>
#include <vtkImageCanvasSource2D.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageStack.h>
#include <vtkImageStencil.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLine.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkMath.h>
#include <vtkNIFTIImageWriter.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkNrrdReader.h>
#include <vtkPicker.h>
#include <vtkPlane.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkPropPicker.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkStripper.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

class InteractorStyleImage : public vtkInteractorStyleImage
{
public:
    static InteractorStyleImage* New();
    vtkTypeMacro(InteractorStyleImage, vtkInteractorStyleImage);

    void OnLeftButtonDown() override
    {
        m_leftButtonPress = true;
        applyPainting();
        vtkInteractorStyleImage::OnLeftButtonDown();
    }

    void OnLeftButtonUp() override
    {
        vtkInteractorStyleImage::OnLeftButtonUp();
        m_leftButtonPress = false;

        m_renderWindow->Render();
    }

    void OnRightButtonDown() override
    {
        updateRadius(5.0);
    }

    void OnMouseMove() override
    {
        moveCursor();
        if (m_leftButtonPress)
            applyPainting();
        m_renderWindow->Render();
    }

    void setContouringImage(vtkImageData* image)
    {
        m_contouringImage = image;
        updateContouringCursor();
    }

    void setRenderer(vtkRenderer* renderer)
    {
        m_renderer = renderer;
    }

    void setRenderWindow(vtkRenderWindow* renderWindow)
    {
        m_renderWindow = renderWindow;
    }

    void setEraseOn(bool on)
    {
        m_eraseOn = on;
    }

    void setContouringImageActor(vtkImageActor* actor)
    {
        m_contouringImageActor = actor;
    }

    void moveCursor()
    {
        int x = GetInteractor()->GetEventPosition()[0];
        int y = GetInteractor()->GetEventPosition()[1];

        vtkNew<vtkCellPicker> picker;
        picker->Pick(x, y, 0.0, m_renderer);
        double pos[3];
        picker->GetPickPosition(pos);
        // Make sure cursor in front of image
        // pos[2] += 0.001;
        qDebug() << "pos[0] = " << pos[0] << ", pos[1] = " << pos[1] << ", pos[2] = " << pos[2];
        m_cursorActor->SetPosition(pos);
        //m_cursorActor->set
    }

    std::vector<std::pair<int, int>> getAdjacentPixelBlocksIndex(const int& imageI, const int& imageJ, const int& blockNum)
    {
        std::vector<std::pair<int, int>> adjacentPixelBlockIndex;
        for (int dx = -blockNum; dx <= blockNum; ++dx)
            for (int dy = -blockNum; dy <= blockNum; ++dy)
                adjacentPixelBlockIndex.push_back({ imageI + dx, imageJ + dy });

        return adjacentPixelBlockIndex;
    }

    void initContouringCursor()
    {
        m_cursorPoints->InsertNextPoint(0.0, 0.0, 0.0);
        m_cursorPoints->InsertNextPoint(1.0, 0.0, 0.0);

        vtkIdType ids[] = { 0, 1 };
        m_cursorCells->InsertNextCell(2, ids);

        m_cursorPolyData->SetPoints(m_cursorPoints);
        m_cursorPolyData->SetLines(m_cursorCells);
        m_cursorMapper->SetInputData(m_cursorPolyData);

        m_cursorActor->SetMapper(m_cursorMapper);
        m_cursorActor->GetProperty()->SetColor(0, 0, 1.0);
        m_renderer->AddActor(m_cursorActor);
    }

    void updateContouringCursor()
    {
        if (!m_contouringImage)
            return;

        double spacing[3];
        m_contouringImage->GetSpacing(spacing);

        double origin[3];
        m_contouringImage->GetOrigin(origin);

        auto adjacentPixelBlocks = calculateAdjacentPixelBlocks(m_radius, spacing[0]);
        auto adjacentPixlBlocksIndex = getAdjacentPixelBlocksIndex(0, 0, adjacentPixelBlocks);
        double basePoint[3] = { origin[0], origin[1], origin[2] };
        auto pixelBlockIndexWithinRadius = getPixelBlockIndexWithinRadius(adjacentPixlBlocksIndex, basePoint, m_radius);

        vtkNew<vtkPoints> points;
        for (auto& item : pixelBlockIndexWithinRadius)
        {
            double point[3] = { 0 };
            point[0] = origin[0] + (item.first * spacing[0]);
            point[1] = origin[1] + (item.second * spacing[1]);
            point[2] = basePoint[2];
            points->InsertNextPoint(point);
        }

        vtkNew<vtkPoints> convexHullPoint;
        //qDebug() << "number of points = " << points->GetNumberOfPoints();
        // vtkNew<vtkPoints> convexHullPoints;
        vtkConvexHull2D::CalculateConvexHull(points, convexHullPoint, spacing[0]);

        // for (int i = 0; i < m_cursorPoints->GetNumberOfPoints(); ++i)
        // {
        //     qDebug() << "x = " << m_cursorPoints->GetPoint(i)[0] << ", y = " << m_cursorPoints->GetPoint(i)[1] << ", z = " << m_cursorPoints->GetPoint(i)[2];
        // }
        vtkNew<vtkCellArray> cells;
        int lineIndex = 0;
        for (; lineIndex < convexHullPoint->GetNumberOfPoints() - 1; lineIndex++)
        {
            vtkIdType id[2] = { lineIndex, lineIndex + 1 };
            qDebug() << "m_linePoints id[0] = " << id[0] << ", id[1] = " << id[1];
            cells->InsertNextCell(2, id);
        }
        vtkIdType lastCell[2] = { 0, convexHullPoint->GetNumberOfPoints() - 1 };
        cells->InsertNextCell(2, lastCell);

        m_cursorPoints = convexHullPoint;
        m_cursorCells = cells;

        m_cursorCells->Modified();
        m_cursorPoints->GetData()->Modified();
        m_cursorPolyData->SetPoints(m_cursorPoints);
        m_cursorPolyData->SetLines(m_cursorCells);
        m_cursorPolyData->Modified();

        m_renderWindow->Render();


        //qDebug() << "number of after points = " << m_cursorPoints->GetNumberOfPoints();
        // for (int i = 0; i < m_cursorPoints->GetNumberOfPoints(); i++)
        // {
        //     qDebug() << "x = " << m_cursorPoints->GetPoint(i)[0] << ", y = " << m_cursorPoints->GetPoint(i)[1];
        // }
    }

    int calculateAdjacentPixelBlocks(const double& radius, const double& spacing)
    {
        return radius / spacing;
    }

    void updateRadius(const double& radius)
    {
        if (m_radius == radius)
            return;

        m_radius = radius;
        updateContouringCursor();
    }

    std::vector<std::pair<int, int>> getPixelBlockIndexWithinRadius(const std::vector<std::pair<int, int>>& validAdjacentPixelBlocksIndex, double basePoint[3], const double& radius)
    {
        std::vector<std::pair<int, int>> pixelBlockIndex;

        double spacing[3] = { 0 };
        m_contouringImage->GetSpacing(spacing);

        double origin[3] = { 0 };
        m_contouringImage->GetOrigin(origin);

        vtkNew<vtkPoints> points;
        for (auto& item : validAdjacentPixelBlocksIndex)
        {
            double x = (item.first * spacing[0]) + origin[0];
            double y = (item.second * spacing[1]) + origin[1];
            double z = basePoint[2];
            points->InsertNextPoint(x, y, z);
        }

        for (int i = 0; i < points->GetNumberOfPoints(); i++)
        {
            auto squaredDistance = vtkMath::Distance2BetweenPoints(points->GetPoint(i), basePoint);
            if (squaredDistance <= (radius * radius))
                pixelBlockIndex.push_back(validAdjacentPixelBlocksIndex[i]);
        }

        return pixelBlockIndex;
    }

    void applyPainting()
    {
        int x = GetInteractor()->GetEventPosition()[0];
        int y = GetInteractor()->GetEventPosition()[1];

        vtkNew<vtkCellPicker> picker;
        // vtkNew<vtkPropPicker> picker;
        picker->Pick(x, y, 0.0, m_renderer);
        auto path = picker->GetPath();
        bool validPick = false;
        vtkImageSlice* imageActor = nullptr;
        if (path)
        {
            vtkCollectionSimpleIterator sit;
            path->InitTraversal(sit);
            // vtkAssemblyNode *node;
            for (int i = 0; i < path->GetNumberOfItems() && !validPick; ++i)
            {
                auto node = path->GetNextNode(sit);
                imageActor = dynamic_cast<vtkImageSlice*>(node->GetViewProp());
                qDebug() << "imageActor->GetMapper()->GetInput() = " << imageActor->GetMapper()->GetInput();
                qDebug() << "m_contouringImage = " << m_contouringImage;
                if (imageActor && imageActor->GetMapper()->GetInput() == m_contouringImage)
                {
                    validPick = true;
                    break;
                }
            }
        }

        // qDebug() << "validPick = " << validPick;

        // if (!validPick)
        //     return;

        double pos[3];
        picker->GetPickPosition(pos);
        // Fixes some numerical problems with the picking.
        // double* bounds = imageActor->GetDisplayBounds();
        // int axis = 2;
        // pos[axis] = bounds[2 * axis];
        // qDebug() << "pick pos x =" << pos[0];
        // qDebug() << "pick pos y =" << pos[1];
        // qDebug() << "pick pos z =" << pos[2];

        double origin[3] = { 0 };
        m_contouringImage->GetOrigin(origin);

        double spacing[3];
        m_contouringImage->GetSpacing(spacing);

        int imageI = (pos[0] - origin[0]) / spacing[0];
        int imageJ = (pos[1] - origin[1]) / spacing[1];
        int imageK = pos[2];

        // qDebug() << "imageI = " << imageI << ", round() = " << (int)imageI;
        // qDebug() << "imageJ = " << imageJ << ", round() = " << (int)imageJ;

        int adjacentPixelBlocks = m_radius / spacing[0];
        std::vector<std::pair<int, int>> adjacentPixelBlocksIndex = getAdjacentPixelBlocksIndex(imageI, imageJ, adjacentPixelBlocks);
        std::vector<std::pair<int, int>> validAdjacentPixelBlocksIndex;

        int extent[6];
        m_contouringImage->GetExtent(extent);
        int minIExtent = extent[0];
        int maxIExtent = extent[1];
        int minJExtent = extent[2];
        int maxJExtent = extent[3];
        for (auto& item : adjacentPixelBlocksIndex)
        {
            if (item.first < minIExtent || item.first > maxIExtent || item.second < minJExtent || item.second > maxJExtent)
                continue;
            validAdjacentPixelBlocksIndex.push_back(item);
        }

        double basePoint[3] = {
            (imageI * spacing[0]) + origin[0],
            (imageJ * spacing[1]) + origin[1],
            pos[2]
        };

        auto pixelBlockIndexWithinRadius = getPixelBlockIndexWithinRadius(validAdjacentPixelBlocksIndex, basePoint, m_radius);
        for (auto& index : pixelBlockIndexWithinRadius)
        {
            double color = m_eraseOn ? 0 : 255.0;
            m_contouringImage->SetScalarComponentFromDouble(index.first, index.second, imageK, 0, color);
        }

        m_contouringImage->Modified();
        m_renderWindow->Render();
    }

private:
    bool m_leftButtonPress = false;
    vtkRenderer* m_renderer = nullptr;
    vtkRenderWindow* m_renderWindow = nullptr;
    vtkImageData* m_contouringImage = nullptr;
    vtkImageActor* m_contouringImageActor = nullptr; 
    bool m_eraseOn = false;
    double m_radius = 10.0;
    vtkSmartPointer<vtkPoints> m_cursorPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> m_cursorCells = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkPolyData> m_cursorPolyData = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPolyDataMapper> m_cursorMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkActor> m_cursorActor = vtkSmartPointer<vtkActor>::New();
};
vtkStandardNewMacro(InteractorStyleImage);

VTKOpenGLWidget::VTKOpenGLWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_renderer(vtkSmartPointer<vtkRenderer>::New())
    , m_interactorStyle(vtkSmartPointer<InteractorStyleImage>::New())
    , m_linePoints(vtkSmartPointer<vtkPoints>::New())
    , m_lineCells(vtkSmartPointer<vtkCellArray>::New())
    , m_lineData(vtkSmartPointer<vtkPolyData>::New())
    , m_baseImage(nullptr)
{
    initialize();
    createTestData();
}

void VTKOpenGLWidget::saveImageToLocal()
{
    if (!m_baseImage)
        return;

    vtkNew<vtkNIFTIImageWriter> writer;
    writer->SetFileName("D:/contouring.nii.gz");
    writer->SetInputData(m_baseImage);
    writer->Update();
}

void VTKOpenGLWidget::setEraseOn(bool on)
{
    m_interactorStyle->setEraseOn(on);
}

void VTKOpenGLWidget::initialize()
{
    m_renderer->SetBackground(1.0, 0.0, 1.0);
    m_renderWindow->AddRenderer(m_renderer);
    SetRenderWindow(m_renderWindow);
    m_interactorStyle->setRenderer(m_renderer);
    m_interactorStyle->setRenderWindow(m_renderWindow);
    m_renderWindow->GetInteractor()->SetInteractorStyle(m_interactorStyle);
}

void VTKOpenGLWidget::createTestData()
{
    vtkNew<vtkNrrdReader> reader;
    reader->SetFileName("D:/UROPRO/patient/-1/-1/Patient20201029MR_T2W_SPAIR_ax.nrrd");
    // reader->SetFileName("D:/MRI.nrrd");
    reader->Update();

    double spacing[3];
    reader->GetDataSpacing(spacing);

    qDebug() << "spacing";
    for (int i = 0; i < 3; ++i)
        qDebug() << spacing[i];

    double origin[3];
    reader->GetDataOrigin(origin);

    qDebug() << "origin";
    for (int i = 0; i < 3; ++i)
        qDebug() << origin[i];

    int extent[6];
    reader->GetDataExtent(extent);

    qDebug() << "extent";
    for (int i = 0; i < 6; ++i)
        qDebug() << extent[i];

    // std::cout << *reader->GetTransform()->GetMatrix();

    vtkNew<vtkImageData> source;
    source->SetSpacing(reader->GetDataSpacing());
    source->SetOrigin(reader->GetDataOrigin());
    source->SetExtent(reader->GetDataExtent());
    source->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
    initColor(source, 0);

    m_baseImage = source;

    vtkNew<vtkImageSliceMapper> imageMapper;
    imageMapper->SetInputData(source);

    vtkNew<vtkImageSlice> slice;
    slice->SetMapper(imageMapper);

    qDebug() << "bounds";
    double bounds[6];
    slice->GetBounds(bounds);
    for (int i = 0; i < 6; i++)
        qDebug() << bounds[i];

    vtkNew<vtkImageStack> stack;
    stack->AddImage(slice);

    m_renderer->AddViewProp(stack);

    m_interactorStyle->initContouringCursor();
    m_interactorStyle->setContouringImage(source);
    // m_interactorStyle->setContouringImageActor(actor);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(m_lineData);

    vtkNew<vtkActor> lineActor;
    lineActor->SetMapper(mapper);
    lineActor->GetProperty()->SetColor(0, 0, 0);
    lineActor->GetProperty()->SetLineWidth(2);
    lineActor->GetProperty()->SetRepresentationToWireframe();
    lineActor->GetProperty()->SetInterpolationToFlat();

    // m_renderer->AddActor(lineActor);

}

void VTKOpenGLWidget::initColor(vtkImageData *image, const int &color)
{
    vtkIdType count = image->GetNumberOfPoints();
    for (vtkIdType i = 0; i < count; ++i)
        image->GetPointData()->GetScalars()->SetTuple1(i, color);
}
