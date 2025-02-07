#include "VTKOpenGLWidget.h"
#include <QDebug>
#include <array>
#include <queue>
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
#include <vtkImageConnectivityFilter.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkImageProperty.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageStack.h>
#include <vtkImageStencil.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLine.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkNIFTIImageWriter.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
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

class InteractorStyleImage : public vtkInteractorStyleImage {
public:
  static InteractorStyleImage *New();
  vtkTypeMacro(InteractorStyleImage, vtkInteractorStyleImage);

  void OnLeftButtonDown() override {
    m_leftButtonPress = true;
    applyPainting();
    vtkInteractorStyleImage::OnLeftButtonDown();
  }

  void OnLeftButtonUp() override {
    vtkInteractorStyleImage::OnLeftButtonUp();
    m_leftButtonPress = false;

    m_renderWindow->Render();
  }

  void OnRightButtonDown() override { updateRadius(5.0); }

  void OnMouseMove() override {
    moveCursor();
    if (m_leftButtonPress)
      applyPainting();
    m_renderWindow->Render();
  }

  void setContouringImage(vtkImageData *image) {
    m_contouringImage = image;
    updateContouringCursor();
  }

  void setRenderer(vtkRenderer *renderer) { m_renderer = renderer; }

  void setRenderWindow(vtkRenderWindow *renderWindow) {
    m_renderWindow = renderWindow;
  }

  void setEraseOn(bool on) { m_eraseOn = on; }

  void setContouringImageActor(vtkImageActor *actor) {
    m_contouringImageActor = actor;
  }

  void moveCursor() {
    int x = GetInteractor()->GetEventPosition()[0];
    int y = GetInteractor()->GetEventPosition()[1];

    vtkNew<vtkCellPicker> picker;
    picker->Pick(x, y, 0.0, m_renderer);
    double pos[3];
    picker->GetPickPosition(pos);
    // Make sure cursor in front of image
    // pos[2] += 0.001;
    m_cursorActor->SetPosition(pos);
    // m_cursorActor->set
  }

  std::vector<std::pair<int, int>>
  getAdjacentPixelBlocksIndex(const int &imageI, const int &imageJ,
                              const int &blockNum) {
    std::vector<std::pair<int, int>> adjacentPixelBlockIndex;
    for (int dx = -blockNum; dx <= blockNum; ++dx)
      for (int dy = -blockNum; dy <= blockNum; ++dy)
        adjacentPixelBlockIndex.push_back({imageI + dx, imageJ + dy});

    return adjacentPixelBlockIndex;
  }

  void initContouringCursor() {
    m_cursorPoints->InsertNextPoint(0.0, 0.0, 0.0);
    m_cursorPoints->InsertNextPoint(1.0, 0.0, 0.0);

    vtkIdType ids[] = {0, 1};
    m_cursorCells->InsertNextCell(2, ids);

    m_cursorPolyData->SetPoints(m_cursorPoints);
    m_cursorPolyData->SetLines(m_cursorCells);
    m_cursorMapper->SetInputData(m_cursorPolyData);

    m_cursorActor->SetMapper(m_cursorMapper);
    m_cursorActor->GetProperty()->SetColor(0, 0, 1.0);
    m_renderer->AddActor(m_cursorActor);
  }

  void updateContouringCursor() {
    if (!m_contouringImage)
      return;

    double spacing[3];
    m_contouringImage->GetSpacing(spacing);

    m_pixelBlocklist = calculatePixelBlockList(m_radius);
    vtkNew<vtkPoints> points;
    for (auto &index : m_pixelBlocklist)
      points->InsertNextPoint(index.first * spacing[0],
                              index.second * spacing[0], 0);

    vtkNew<vtkPoints> convexHullPoint;
    vtkConvexHull2D::CalculateConvexHull(points, convexHullPoint, spacing[0]);

    vtkNew<vtkCellArray> cells;
    int lineIndex = 0;
    for (; lineIndex < convexHullPoint->GetNumberOfPoints() - 1; lineIndex++) {
      vtkIdType id[2] = {lineIndex, lineIndex + 1};
      cells->InsertNextCell(2, id);
    }
    vtkIdType lastCell[2] = {0, convexHullPoint->GetNumberOfPoints() - 1};
    cells->InsertNextCell(2, lastCell);

    m_cursorPoints = convexHullPoint;
    m_cursorCells = cells;

    m_cursorCells->Modified();
    m_cursorPoints->GetData()->Modified();
    m_cursorPolyData->SetPoints(m_cursorPoints);
    m_cursorPolyData->SetLines(m_cursorCells);
    m_cursorPolyData->Modified();

    m_renderWindow->Render();
  }

  int calculateAdjacentPixelBlocks(const double &radius,
                                   const double &spacing) {
    return radius / spacing;
  }

  void updateRadius(const double &radius) {
    if (m_radius == radius)
      return;

    m_radius = radius;
    updateContouringCursor();
  }

  std::vector<std::pair<int, int>> getPixelBlockIndexWithinRadius(
      const std::vector<std::pair<int, int>> &validAdjacentPixelBlocksIndex,
      double basePoint[3], const double &radius) {
    std::vector<std::pair<int, int>> pixelBlockIndex;

    double spacing[3] = {0};
    m_contouringImage->GetSpacing(spacing);

    double origin[3] = {0};
    m_contouringImage->GetOrigin(origin);

    vtkNew<vtkPoints> points;
    for (auto &item : validAdjacentPixelBlocksIndex) {
      double x = (item.first * spacing[0]) + origin[0];
      double y = (item.second * spacing[1]) + origin[1];
      double z = basePoint[2];
      points->InsertNextPoint(x, y, z);
    }

    for (int i = 0; i < points->GetNumberOfPoints(); i++) {
      auto squaredDistance =
          vtkMath::Distance2BetweenPoints(points->GetPoint(i), basePoint);
      if (squaredDistance <= (radius * radius))
        pixelBlockIndex.push_back(validAdjacentPixelBlocksIndex[i]);
    }

    return pixelBlockIndex;
  }

  void applyPainting() {
    int x = GetInteractor()->GetEventPosition()[0];
    int y = GetInteractor()->GetEventPosition()[1];

    vtkNew<vtkCellPicker> picker;
    // vtkNew<vtkPropPicker> picker;
    picker->Pick(x, y, 0.0, m_renderer);
    auto path = picker->GetPath();
    bool validPick = false;
    vtkImageSlice *imageActor = nullptr;
    if (path) {
      vtkCollectionSimpleIterator sit;
      path->InitTraversal(sit);
      // vtkAssemblyNode *node;
      for (int i = 0; i < path->GetNumberOfItems() && !validPick; ++i) {
        auto node = path->GetNextNode(sit);
        imageActor = dynamic_cast<vtkImageSlice *>(node->GetViewProp());
        if (imageActor &&
            imageActor->GetMapper()->GetInput() == m_contouringImage) {
          validPick = true;
          break;
        }
      }
    }

    if (!validPick)
      return;

    double pos[3];
    picker->GetPickPosition(pos);

    double origin[3] = {0};
    m_contouringImage->GetOrigin(origin);

    double spacing[3];
    m_contouringImage->GetSpacing(spacing);

    int imageI = (pos[0] - origin[0]) / spacing[0];
    int imageJ = (pos[1] - origin[1]) / spacing[1];
    int imageK = pos[2];

    int adjacentPixelBlocks = m_radius / spacing[0];
    std::vector<std::pair<int, int>> adjacentPixelBlocksIndex =
        getAdjacentPixelBlocksIndex(imageI, imageJ, adjacentPixelBlocks);
    std::vector<std::pair<int, int>> validAdjacentPixelBlocksIndex;

    int extent[6];
    m_contouringImage->GetExtent(extent);
    int minIExtent = extent[0];
    int maxIExtent = extent[1];
    int minJExtent = extent[2];
    int maxJExtent = extent[3];
    for (auto &item : adjacentPixelBlocksIndex) {
      if (item.first < minIExtent || item.first > maxIExtent ||
          item.second < minJExtent || item.second > maxJExtent)
        continue;
      validAdjacentPixelBlocksIndex.push_back(item);
    }

    double basePoint[3] = {(imageI * spacing[0]) + origin[0],
                           (imageJ * spacing[1]) + origin[1], pos[2]};

    auto pixelBlockIndexWithinRadius = getPixelBlockIndexWithinRadius(
        validAdjacentPixelBlocksIndex, basePoint, m_radius);
    for (auto &index : pixelBlockIndexWithinRadius) {
      double color = m_eraseOn ? 0 : 1.0;
      m_contouringImage->SetScalarComponentFromDouble(index.first, index.second,
                                                      imageK, 0, color);

      // qDebug() << "x = " << index.first << ", y = " << index.second;
    }

    m_contouringImage->Modified();
    m_renderWindow->Render();
  }

private:
  std::vector<std::pair<int, int>>
  calculatePixelBlockList(const double &radius);

  bool m_leftButtonPress = false;
  vtkRenderer *m_renderer = nullptr;
  vtkRenderWindow *m_renderWindow = nullptr;
  vtkImageData *m_contouringImage = nullptr;
  vtkImageActor *m_contouringImageActor = nullptr;
  bool m_eraseOn = false;
  double m_radius = 10.0;
  std::vector<std::pair<int, int>> m_pixelBlocklist;
  vtkSmartPointer<vtkPoints> m_cursorPoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> m_cursorCells =
      vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkPolyData> m_cursorPolyData =
      vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPolyDataMapper> m_cursorMapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkActor> m_cursorActor = vtkSmartPointer<vtkActor>::New();
};
vtkStandardNewMacro(InteractorStyleImage);

VTKOpenGLWidget::VTKOpenGLWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent),
      m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
      m_renderer(vtkSmartPointer<vtkRenderer>::New()),
      m_interactorStyle(vtkSmartPointer<InteractorStyleImage>::New()),
      m_baseImage(nullptr) {
  initialize();
  createTestData();
}

void VTKOpenGLWidget::saveImageToLocal() {
  if (!m_baseImage)
    return;

  vtkNew<vtkNIFTIImageWriter> writer;
  writer->SetFileName("D:/contouring.nii.gz");
  writer->SetInputData(m_baseImage);
  writer->Update();
}

void VTKOpenGLWidget::setEraseOn(bool on) { m_interactorStyle->setEraseOn(on); }

void VTKOpenGLWidget::initialize() {
  m_renderer->SetBackground(1.0, 0.0, 1.0);
  m_renderWindow->AddRenderer(m_renderer);
  SetRenderWindow(m_renderWindow);
  m_interactorStyle->setRenderer(m_renderer);
  m_interactorStyle->setRenderWindow(m_renderWindow);
  m_renderWindow->GetInteractor()->SetInteractorStyle(m_interactorStyle);
}

void VTKOpenGLWidget::createTestData() {
  int extent[6] = {0, 199, 0, 199, 0, 99};
  vtkNew<vtkImageData> source;
  source->SetSpacing(1.0, 1.0, 1.0);
  source->SetOrigin(0, 0, 0);
  source->SetExtent(extent);
  source->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
  initColor(source, 0);

  m_baseImage = source;

  vtkNew<vtkLookupTable> lookupTable;
  lookupTable->SetHueRange(18.0 / 360.0, 18.0 / 360.0);
  lookupTable->SetSaturationRange(0.3333, 0.3333);
  lookupTable->SetValueRange(0, 0.9412);
  lookupTable->SetAlphaRange(1, 1);
  lookupTable->Build();

  vtkNew<vtkImageActor> imageActor;
  imageActor->SetInputData(source);
  imageActor->GetProperty()->UseLookupTableScalarRangeOn();
  imageActor->GetProperty()->SetLookupTable(lookupTable);
  m_renderer->AddViewProp(imageActor);

  m_interactorStyle->initContouringCursor();
  m_interactorStyle->setContouringImage(source);
}

void VTKOpenGLWidget::initColor(vtkImageData *image, const int &color) {
  vtkIdType count = image->GetNumberOfPoints();
  for (vtkIdType i = 0; i < count; ++i)
    image->GetPointData()->GetScalars()->SetTuple1(i, color);
}

void VTKOpenGLWidget::autoFill() {
  if (!m_baseImage)
    return;

  vtkNew<vtkImageConnectivityFilter> connectivity;
  connectivity->SetInputData(m_baseImage);
  connectivity->SetScalarRange(1, 1);
  connectivity->SetExtractionModeToAllRegions();
  connectivity->GenerateRegionExtentsOn();
  connectivity->Update();

  auto regionNum = connectivity->GetNumberOfExtractedRegions();
  qDebug() << "regionNum = " << regionNum;
  if (regionNum == 0)
    return;

  auto extentArray = connectivity->GetExtractedRegionExtents();
  if (!connectivity->GetGenerateRegionExtents())
    return;

  std::array<int, 6> imageExtent;
  m_baseImage->GetExtent(imageExtent.data());

  const int EXTENT_SIZE = 6;
  std::array<int, EXTENT_SIZE> holeExtent;
  for (int i = 0; i < regionNum; ++i) {
    holeExtent[0] = extentArray->GetValue(EXTENT_SIZE * i);
    holeExtent[1] = extentArray->GetValue(EXTENT_SIZE * i + 1);
    holeExtent[2] = extentArray->GetValue(EXTENT_SIZE * i + 2);
    holeExtent[3] = extentArray->GetValue(EXTENT_SIZE * i + 3);
    holeExtent[4] = extentArray->GetValue(EXTENT_SIZE * i + 4);
    holeExtent[5] = extentArray->GetValue(EXTENT_SIZE * i + 5);

    int maxHoleExtentX = holeExtent[1];
    int minHoleExtentX = holeExtent[0];
    int maxHoleExtentY = holeExtent[3];
    int minHoleExtentY = holeExtent[2];

    int centerExtentX = (maxHoleExtentX - minHoleExtentX) / 2 + minHoleExtentX;
    int centerExtentY = (maxHoleExtentY - minHoleExtentY) / 2 + minHoleExtentY;

    std::vector<std::vector<bool>> visited(
        imageExtent[3], std::vector<bool>(imageExtent[1], false));

    std::queue<std::pair<int, int>> q;

    std::vector<std::pair<int, int>> toDrawArea;

    visited[centerExtentX][centerExtentY] = true;
    q.push({centerExtentX, centerExtentY});
    toDrawArea.push_back({centerExtentX, centerExtentY});

    std::vector<std::pair<int, int>> direction = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    bool result = true;
    while (!q.empty()) {
      std::pair<int, int> current = q.front();
      q.pop();

      for (auto &dir : direction) {
        int x = current.first + dir.first;
        int y = current.second + dir.second;

        auto color =
            m_baseImage->GetScalarComponentAsDouble(x, y, holeExtent[4], 0);

        if (color == 0.0 && (x >= maxHoleExtentX || y >= maxHoleExtentY ||
                             x <= minHoleExtentX || y <= minHoleExtentY)) {
          result = false;
          break;
        } else if (color == 0.0 && !visited[x][y]) {
          visited[x][y] = true;
          q.push({x, y});
          toDrawArea.push_back({x, y});
        }
      }
    }

    if (result && toDrawArea.size() > 1) {
      qDebug() << "There is a hole";

      qDebug() << toDrawArea.size();
      for (auto &item : toDrawArea) {
        m_baseImage->SetScalarComponentFromDouble(item.first, item.second,
                                                  holeExtent[4], 0, 1.0);
      }
      m_baseImage->Modified();

    } else {
      qDebug() << "There is no hole!";
    }
  }

  m_renderWindow->Render();
}

std::vector<std::pair<int, int>>
InteractorStyleImage::calculatePixelBlockList(const double &radius) {
  std::vector<std::pair<int, int>> pixelBlockList;

  double spacing[3];
  m_contouringImage->GetSpacing(spacing);

  int adjacentPixelBlocks = radius / spacing[0];
  auto adjacentPixelBlockIndex =
      getAdjacentPixelBlocksIndex(0, 0, adjacentPixelBlocks);
  vtkNew<vtkPoints> points;
  for (auto &index : adjacentPixelBlockIndex)
    points->InsertNextPoint(index.first * spacing[0], index.second * spacing[0],
                            0);

  double basePoint[3] = {0};
  for (int i = 0; i < points->GetNumberOfPoints(); i++) {
    double squaredDistance =
        vtkMath::Distance2BetweenPoints(basePoint, points->GetPoint(i));
    if (squaredDistance <= (radius * radius))
      pixelBlockList.push_back(adjacentPixelBlockIndex[i]);
  }

  return pixelBlockList;
}
