#include "VTKOpenGLWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkSphereSource.h>
#include <vtkCutter.h>
#include <vtkPlane.h>
#include <vtkStripper.h>
#include <vtkImageData.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkImageStencil.h>
#include <vtkPointData.h>
#include <vtkImageActor.h>
#include <vtkInteractorStyleImage.h>
#include <QDebug>
#include <vtkCoordinate.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNamedColors.h>
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkCamera.h>
#include <vtkAppendPolyData.h>
#include <vtkGlyph3D.h>
#include <vtkCleanPolyData.h>
#include <vtkPicker.h>
#include <vtkAbstractPicker.h>
#include <vtkAssemblyPath.h>
#include <vtkPropPicker.h>
#include <vtkPicker.h>
#include <vtkCellPicker.h>
#include <vtkMath.h>
#include <vtkNrrdReader.h>
#include <vtkImageCanvasSource2D.h>
#include <array>
#include <vtkMath.h>
#include <vtkNIFTIImageWriter.h>

int a = 1;
int b = 1;
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

    qDebug() << "validPick = " << validPick;

    if (!validPick)
        return;

    double pos[3];
    picker->GetPickPosition(pos);
    // Fixes some numerical problems with the picking.
    double* bounds = imageActor->GetDisplayBounds();
    int axis = 2;
    pos[axis] = bounds[2 * axis];
    qDebug() << "pick pos x =" << pos[0];
    qDebug() << "pick pos y =" << pos[1];
    qDebug() << "pick pos z =" << pos[2];

    const double radius = 1.5;

    double origin[3] = { 0 };
    m_baseImage->GetOrigin(origin);

    double spacing[3] = { 0 };
    m_baseImage->GetSpacing(spacing);
    int imageI = (pos[0] - origin[0]) / spacing[0];
    int imageJ = (pos[1] - origin[1]) / spacing[1];
    int imageK = pos[2];

    qDebug() << "imageI = " << imageI << ", round() = " << (int)imageI;
    qDebug() << "imageJ = " << imageJ << ", round() = " << (int)imageJ;

    int adjacentBlocks = radius / spacing[0];
    // qDebug()<<"AdjacentBlocks = "<<adjacentBlocks;

    std::vector<std::pair<int, int>> blockIndexs;
    for (int dx = -adjacentBlocks; dx <= adjacentBlocks; ++dx)
        for (int dy = -adjacentBlocks; dy <= adjacentBlocks; ++dy)
            blockIndexs.push_back({ imageI + dx, imageJ + dy });

    std::vector<std::pair<int, int>> filterBlockIndex;
    for (auto& item : blockIndexs)
    {
        if (item.first >= 0 && item.second >= 0)
            filterBlockIndex.push_back(item);
    }

    qDebug() << "filterBlockIndex size = " << blockIndexs.size();
    for (auto& item : filterBlockIndex)
    {
        qDebug() << "\n";
        qDebug() << "x = " << item.first;
        qDebug() << "y = " << item.second;
    }

    qDebug() << "Components number = " << m_baseImage->GetNumberOfScalarComponents();

    double basePoint[3] = {
        (imageI * spacing[0]) + origin[0],
        (imageJ * spacing[1]) + origin[1],
        pos[2]
    };

    vtkNew<vtkPoints> points;
    for (auto& item : filterBlockIndex)
    {
        double x = (item.first * spacing[0]) + origin[0];
        double y = (item.second * spacing[1]) + origin[1];
        double z = pos[2];
        points->InsertNextPoint(x, y, z);
    }

    std::vector<std::pair<int, int>> validBlockIndexs;
    for (int i = 0; i < points->GetNumberOfPoints(); i++)
    {
        auto squaredDistance = vtkMath::Distance2BetweenPoints(points->GetPoint(i), basePoint);
        if (squaredDistance <= radius)
        {
            validBlockIndexs.push_back(filterBlockIndex[i]);
        }
    }

    for (auto& index : validBlockIndexs)
    {
        m_baseImage->SetScalarComponentFromDouble(index.first, index.second, imageK, 0, 255);
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

void VTKOpenGLWidget::initialize()
{
    m_renderer->SetBackground(1.0, 1.0, 0.0);
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
    m_interactorStyle->setLinePoints(m_linePoints);
    m_interactorStyle->setLineCells(m_lineCells);
    m_interactorStyle->setLineData(m_lineData);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(m_lineData);

    vtkNew<vtkActor> lineActor;
    lineActor->SetMapper(mapper);
    lineActor->GetProperty()->SetColor(0, 0, 0);
    lineActor->GetProperty()->SetLineWidth(2);
    lineActor->GetProperty()->SetRepresentationToWireframe();
    lineActor->GetProperty()->SetInterpolationToFlat();

    m_renderer->AddActor(lineActor);
    m_renderer->AddViewProp(actor);
}

void VTKOpenGLWidget::initColor(vtkImageData *image, const int &color)
{
    vtkIdType count = image->GetNumberOfPoints();
    for (vtkIdType i = 0; i < count; ++i)
        image->GetPointData()->GetScalars()->SetTuple1(i, color);
}