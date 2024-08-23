#include "VTKOpenGLWidget.h"
#include <QDebug>
#include <array>
#include <vtkAbstractPicker.h>
#include <vtkActor.h>
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
#include <vtkPolyDataToImageStencil.h>
#include <vtkPropPicker.h>
#include <vtkProperty.h>
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

    void OnMouseMove() override
    {
        if (m_leftButtonPress)
            applyPainting();
    }

    void setContouringImage(vtkImageData* image)
    {
        m_contouringImage = image;
    }

    void setRenderer(vtkRenderer* renderer)
    {
        m_renderer = renderer;
    }

    void setRenderWindow(vtkRenderWindow* renderWindow)
    {
        m_renderWindow = renderWindow;
    }

    void setBaseImage(vtkImageData* image)
    {
        m_baseImage = image;
    }

    void setLinePoints(vtkPoints* linePoints)
    {
        m_linePoints = linePoints;
    }

    void setLineCells(vtkCellArray* lineCells)
    {
        m_lineCells = lineCells;
    }

    void setLineData(vtkPolyData* lineData)
    {
        m_lineData = lineData;
    }

    void resetLine(double* pos)
    {
        this->m_pickCount = 0;
        this->m_linePoints->InsertPoint(m_pickCount, pos);
    }

    void setEraseOn(bool on)
    {
        m_eraseOn = on;
    }

    void setImageSpacing(double imageSpacing[3])
    {
        for (int i = 0; i < 3; i++)
            m_imageSpacing[i] = imageSpacing[i];
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
        auto adjacentPixelBlocks = calculateAdjacentPixelBlocks(m_radius, m_imageSpacing[0]);
        auto adjacentPixlBlocksIndex = getAdjacentPixelBlocksIndex(0, 0, adjacentPixelBlocks);
        double basePoint[3] = { 0 };
        auto pixelBlockIndexWithinRadius = getPixelBlockIndexWithinRadius(adjacentPixlBlocksIndex, basePoint, m_radius);
        double origin[3];
        m_baseImage->GetOrigin(origin);

        double spacing[3];
        m_baseImage->GetSpacing(spacing);

        vtkNew<vtkPoints> points;
        for (auto& item : pixelBlockIndexWithinRadius)
        {
            double point[3] = { 0 };
            point[0] = origin[0] + (item.first * spacing[0]);
            point[1] = origin[1] + (item.second * spacing[1]);
            point[2] = basePoint[2];
            points->InsertNextPoint(point);
        }

        qDebug() << "number of points = " << points->GetNumberOfPoints();
        // vtkNew<vtkPoints> convexHullPoints;
        vtkConvexHull2D::CalculateConvexHull(points, m_linePoints, spacing[0]);

        for (int i = 0; i < m_linePoints->GetNumberOfPoints(); ++i)
        {
            qDebug() << "x = " << m_linePoints->GetPoint(i)[0] << ", y = " << m_linePoints->GetPoint(i)[1] << ", z = " << m_linePoints->GetPoint(i)[2];
        }
        int lineIndex = 0;
        for (; lineIndex < m_linePoints->GetNumberOfPoints() - 1; lineIndex++)
        {
            vtkIdType id[2] = { lineIndex, lineIndex + 1 };
            qDebug() << "m_linePoints id[0] = " << id[0] << ", id[1] = " << id[1];
            m_lineCells->InsertNextCell(2, id);
        }
        vtkIdType lastCell[2] = { 0, m_linePoints->GetNumberOfPoints() - 1 };
        m_lineCells->InsertNextCell(2, lastCell);

        qDebug() << "number of after points = " << m_linePoints->GetNumberOfPoints();

        for (int i = 0; i < m_linePoints->GetNumberOfPoints(); i++)
        {
            qDebug() << "x = " << m_linePoints->GetPoint(i)[0] << ", y = " << m_linePoints->GetPoint(i)[1];
        }

        m_lineData->SetPoints(m_linePoints);
        m_lineData->SetLines(m_lineCells);

        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputData(m_lineData);

        vtkNew<vtkActor> lineActor;
        lineActor->SetMapper(mapper);
        lineActor->GetProperty()->SetColor(0, 0, 0);
        lineActor->GetProperty()->SetLineWidth(2);
        m_renderer->AddActor(lineActor);
    }

    int calculateAdjacentPixelBlocks(const double& radius, const double& spacing)
    {
        return radius / spacing;
    }

    std::vector<std::pair<int, int>> getPixelBlockIndexWithinRadius(const std::vector<std::pair<int, int>>& validAdjacentPixelBlocksIndex, double basePoint[3], const double& radius)
    {
        std::vector<std::pair<int, int>> pixelBlockIndex;

        double spacing[3] = { 0 };
        m_baseImage->GetSpacing(spacing);

        double origin[3] = { 0 };
        m_baseImage->GetOrigin(origin);

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
        picker->Pick(x, y, 0.0, m_renderer);
        auto path = picker->GetPath();
        bool validPick = false;
        vtkImageActor* imageActor = nullptr;
        if (path)
        {
            vtkCollectionSimpleIterator sit;
            path->InitTraversal(sit);
            // vtkAssemblyNode *node;
            for (int i = 0; i < path->GetNumberOfItems() && !validPick; ++i)
            {
                auto node = path->GetNextNode(sit);
                imageActor = dynamic_cast<vtkImageActor*>(node->GetViewProp());
                if (imageActor)
                {
                    validPick = true;
                    break;
                }
            }
        }

        // qDebug() << "validPick = " << validPick;

        if (!validPick)
            return;

        double pos[3];
        picker->GetPickPosition(pos);
        // Fixes some numerical problems with the picking.
        double* bounds = imageActor->GetDisplayBounds();
        int axis = 2;
        pos[axis] = bounds[2 * axis];
        // qDebug() << "pick pos x =" << pos[0];
        // qDebug() << "pick pos y =" << pos[1];
        // qDebug() << "pick pos z =" << pos[2];

        double origin[3] = { 0 };
        m_baseImage->GetOrigin(origin);

        int imageI = (pos[0] - origin[0]) / m_imageSpacing[0];
        int imageJ = (pos[1] - origin[1]) / m_imageSpacing[1];
        int imageK = pos[2];

        // qDebug() << "imageI = " << imageI << ", round() = " << (int)imageI;
        // qDebug() << "imageJ = " << imageJ << ", round() = " << (int)imageJ;

        int adjacentPixelBlocks = m_radius / m_imageSpacing[0];
        std::vector<std::pair<int, int>> adjacentPixelBlocksIndex = getAdjacentPixelBlocksIndex(imageI, imageJ, adjacentPixelBlocks);
        std::vector<std::pair<int, int>> validAdjacentPixelBlocksIndex;

        int extent[6];
        m_baseImage->GetExtent(extent);
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
            (imageI * m_imageSpacing[0]) + origin[0],
            (imageJ * m_imageSpacing[1]) + origin[1],
            pos[2]
        };

        auto pixelBlockIndexWithinRadius = getPixelBlockIndexWithinRadius(validAdjacentPixelBlocksIndex, basePoint, m_radius);
        for (auto& index : pixelBlockIndexWithinRadius)
        {
            double color = m_eraseOn ? 0 : 255.0;
            m_baseImage->SetScalarComponentFromDouble(index.first, index.second, imageK, 0, color);
        }

        m_baseImage->Modified();
        m_renderWindow->Render();
    }

private:
    int m_lastEventPosition[2] = { 0 };
    bool m_leftButtonPress = false;
    vtkImageData* m_contouringImage = nullptr;
    vtkRenderer* m_renderer = nullptr;
    vtkRenderWindow* m_renderWindow;
    vtkImageData* m_baseImage;
    vtkPoints* m_linePoints;
    vtkCellArray* m_lineCells;
    vtkPolyData* m_lineData;
    int m_pickCount = 0;
    vtkIdType currentPoints[2];
    bool m_eraseOn = false;
    double m_radius = 2.5;
    std::array<double, 3> m_imageSpacing;
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
    int extent[6] = {0, 199, 0, 199, 0, 99};
    vtkNew<vtkImageData> source;
    source->SetSpacing(1.0, 1.0, 1.0);
    source->SetOrigin(0, 0, 0);
    source->SetExtent(extent);
    source->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
    initColor(source, 0);

    m_baseImage = source;

    vtkNew<vtkImageActor> actor;
    actor->SetInputData(source);

    m_interactorStyle->setBaseImage(source);
    m_interactorStyle->setImageSpacing(source->GetSpacing());
    m_interactorStyle->setLinePoints(m_linePoints);
    m_interactorStyle->setLineCells(m_lineCells);
    m_interactorStyle->setLineData(m_lineData);
    m_interactorStyle->initContouringCursor();

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(m_lineData);

    vtkNew<vtkActor> lineActor;
    lineActor->SetMapper(mapper);
    lineActor->GetProperty()->SetColor(0, 0, 0);
    lineActor->GetProperty()->SetLineWidth(2);
    lineActor->GetProperty()->SetRepresentationToWireframe();
    lineActor->GetProperty()->SetInterpolationToFlat();

    // m_renderer->AddActor(lineActor);
    // m_renderer->AddViewProp(actor);
}

void VTKOpenGLWidget::initColor(vtkImageData *image, const int &color)
{
    vtkIdType count = image->GetNumberOfPoints();
    for (vtkIdType i = 0; i < count; ++i)
        image->GetPointData()->GetScalars()->SetTuple1(i, color);
}
