#include <vtkActor.h>
#include <vtkCursor2D.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkCursor3D.h>
#include <vtkLookupTable.h>
#include <vtkResliceCursor.h>
#include <vtkResliceCursorActor.h>
#include <vtkResliceCursorPolyDataAlgorithm.h>
#include <vtkNrrdReader.h>
#include <vtkImageSlice.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>
#include <vtkCallbackCommand.h>
#include <vtkInteractorStyleImage.h>
#include <vtkCamera.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSlice.h>
#include <vector>

enum class ViewerPosition {
    BOTTOM_LEFT,
    BOTTOM_MIDDLE,
    BOTTOM_RIGHT,
    TOP_LEFT,
    TOP_MIDDLE,
    TOP_RIGHT,
    VIEWER_POSITION_MAX,
};

class MoveFocusPoint : public vtkCallbackCommand
{
public:
    static MoveFocusPoint* New()
    {
        return new MoveFocusPoint;
    }

    void Execute(vtkObject* caller, unsigned long evId, void*) override
    {
        auto interactor = reinterpret_cast<vtkRenderWindowInteractor*>(caller);
        int x = interactor->GetEventPosition()[0];
        int y = interactor->GetEventPosition()[1];
        int z = interactor->GetEventPosition()[2];

        mResliceCursor->SetCenter(x, y, 0);
        mRenderWindow->Render();
        //std::cout << "x=" << x << "y=" << y << "z=" << z << endl;
    }

    void SetResliceCursor(vtkResliceCursor* resliceCursor)
    {
        mResliceCursor = resliceCursor;
    }

    void SetRenderWindow(vtkRenderWindow* rendererWindow)
    {
        mRenderWindow = rendererWindow;
    }
private:
    vtkResliceCursor* mResliceCursor;
    vtkRenderWindow* mRenderWindow;
};

int main(int, char*[])
{
    vtkNew<vtkNamedColors> colors;

    vtkNew<vtkNrrdReader> reader;
    reader->SetFileName("D:/MRI.nrrd");
    reader->Update();

    std::vector<vtkImageResliceMapper*> mappers;
    for (size_t i = 0; i < (size_t)ViewerPosition::VIEWER_POSITION_MAX; i++)
    {
        vtkImageResliceMapper* mapper = vtkImageResliceMapper::New();
        mapper->SetInputConnection(reader->GetOutputPort());
        mapper->SliceFacesCameraOn();
        mappers.push_back(mapper);
    }

    std::vector<vtkImageSlice*> imageSlices;
    for (size_t i = 0; i < (size_t)ViewerPosition::VIEWER_POSITION_MAX; i++)
    {
        vtkImageSlice* imageSlice = vtkImageSlice::New();
        imageSlice->SetMapper(mappers[i]);
        imageSlices.push_back(imageSlice);
    }

    std::vector<vtkRenderer*> renderers;
    for (size_t i = 0; i < (size_t)ViewerPosition::VIEWER_POSITION_MAX; i++)
    {
        vtkRenderer* renderer = vtkRenderer::New();
        renderer->SetBackground(colors->GetColor3d("MidnightBlue").GetData());
        renderers.push_back(renderer);
    }

    // y positive direction is Up
    renderers[(int)ViewerPosition::BOTTOM_LEFT]->SetViewport(0, 0, 0.33, 0.5);
    renderers[(int)ViewerPosition::BOTTOM_MIDDLE]->SetViewport(0.33, 0, 0.66, 0.5);
    renderers[(int)ViewerPosition::BOTTOM_RIGHT]->SetViewport(0.66, 0, 1.0, 0.5); 
    renderers[(int)ViewerPosition::TOP_LEFT]->SetViewport(0, 0.5, 0.33, 1.0);
    renderers[(int)ViewerPosition::TOP_MIDDLE]->SetViewport(0.33, 0.5, 0.66, 1.0);
    renderers[(int)ViewerPosition::TOP_RIGHT]->SetViewport(0.66, 0.5, 1.0, 1.0); 

    vtkNew<vtkRenderWindow> renderWindow;
    renderWindow->SetWindowName("Cursor2D");
    for (size_t i = 0; i < renderers.size(); i++)
    {
        renderWindow->AddRenderer(renderers[i]);
    }

    vtkNew<vtkResliceCursor> cursor;
    cursor->SetCenter(0, 0, 0);
    cursor->SetImage(reader->GetOutput());
    // hide the hole
    cursor->SetHole(0);

    vtkNew<MoveFocusPoint> moveFocusPoint;
    moveFocusPoint->SetResliceCursor(cursor);
    moveFocusPoint->SetRenderWindow(renderWindow);

    vtkNew<vtkInteractorStyleImage> styleImage;
    vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
    renderWindowInteractor->SetRenderWindow(renderWindow);
    renderWindowInteractor->SetInteractorStyle(styleImage);
    renderWindowInteractor->AddObserver(vtkCommand::MouseMoveEvent, moveFocusPoint);

    // Add the actor to the scene
    for (size_t i = 0; i < imageSlices.size(); ++i)
    {
        renderers[i]->AddViewProp(imageSlices[i]);
    }

    std::vector<vtkResliceCursorActor*> cursorActors;
    for (size_t i = 0; i < (size_t)ViewerPosition::VIEWER_POSITION_MAX; i++)
    {
        vtkResliceCursorActor* cursorActor = vtkResliceCursorActor::New();
        cursorActor->GetCursorAlgorithm()->SetResliceCursor(cursor);
        renderers[i]->AddActor(cursorActor);
        cursorActors.push_back(cursorActor);
    }

    cursorActors[(int)ViewerPosition::BOTTOM_LEFT]->GetCursorAlgorithm()->SetReslicePlaneNormalToXAxis();
    cursorActors[(int)ViewerPosition::BOTTOM_MIDDLE]->GetCursorAlgorithm()->SetReslicePlaneNormalToYAxis();
    cursorActors[(int)ViewerPosition::BOTTOM_RIGHT]->GetCursorAlgorithm()->SetReslicePlaneNormalToZAxis(); 
    cursorActors[(int)ViewerPosition::TOP_LEFT]->GetCursorAlgorithm()->SetReslicePlaneNormalToXAxis();
    cursorActors[(int)ViewerPosition::TOP_MIDDLE]->GetCursorAlgorithm()->SetReslicePlaneNormalToYAxis();
    cursorActors[(int)ViewerPosition::TOP_RIGHT]->GetCursorAlgorithm()->SetReslicePlaneNormalToZAxis(); 

    // Set up camera observation's position
    renderers[(int)ViewerPosition::BOTTOM_LEFT]->GetActiveCamera()->Yaw(90);

    renderers[(int)ViewerPosition::BOTTOM_MIDDLE]->GetActiveCamera()->Elevation(90);
    renderers[(int)ViewerPosition::BOTTOM_MIDDLE]->GetActiveCamera()->SetViewUp(0, 0, -1);

    for (size_t i = 0; i < renderers.size(); ++i)
    {
        renderers[i]->ResetCamera();
    }

    // Render and interact
    renderWindow->Render();
    renderWindowInteractor->Start();

    return EXIT_SUCCESS;
}
