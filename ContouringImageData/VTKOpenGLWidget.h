#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>

#include <array>

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
    std::array<int, 4> m_dx;
    std::array<int, 4> m_dy;
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
    std::vector<std::array<int, 6>> getDrawnExtents();

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<InteractorStyleImage> m_interactorStyle;
    vtkImageData *m_baseImage;
};