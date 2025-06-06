#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class InteractorStyleImage;
class vtkPoints;
class vtkCellArray;
class vtkPolyData;
class vtkImageData;

class HoleDetector {
public:
    HoleDetector(const std::vector<std::vector<int>> &inputGrid);
    std::vector<std::pair<int, int>> detectHoles();

private:
    bool isValid(int x, int y);
    void floodFillBFS(int startX, int startY);

    std::vector<std::vector<int>> m_grid;
    std::vector<std::vector<bool>> m_visited;
    int m_rows;
    int m_cols;

    // Directions for 4-connectivity (up, down, left, right)
    int dx[4] = {-1, 1, 0, 0};
    int dy[4] = {0, 0, -1, 1};
};
class VTKOpenGLWidget : public QVTKOpenGLNativeWidget {
public:
    VTKOpenGLWidget(QWidget *parent = nullptr);
    void saveImageToLocal();
    void setEraseOn(bool on);
    void autoFill();

private:
    void initialize();
    void createTestData();
    void initColor(vtkImageData *image, const int &color);
    std::vector<std::array<int, 6>> detectPotentialImageHoles();
    void floodFill(int startX, int startY, std::vector<std::vector<int>> &grid);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<InteractorStyleImage> m_interactorStyle;
    vtkImageData *m_baseImage;
};