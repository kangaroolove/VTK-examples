/*=========================================================================

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// First include the required header files for the VTK classes we are using.
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkConeSource.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>

class KeyPressInteractorStyle : public vtkInteractorStyleTrackballCamera {
public:
    static KeyPressInteractorStyle *New();
    vtkTypeMacro(KeyPressInteractorStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnKeyPress() override {
        vtkRenderWindowInteractor *rwi = this->Interactor;
        std::string key = rwi->GetKeySym();

        std::cout << "Pressed " << key << endl;

        vtkNew<vtkTransform> transform;
        vtkLinearTransform *userTransform = mConeActor->GetUserTransform();
        if (key == "Up") {
            transform->SetInput(userTransform);
            transform->Translate(0, 1, 0);
            mConeActor->SetUserTransform(transform);
        } else if (key == "Down") {
            transform->SetInput(userTransform);
            transform->Translate(0, -1, 0);
            mConeActor->SetUserTransform(transform);
        } else if (key == "Left") {
            transform->SetInput(userTransform);
            transform->Translate(-1, 0, 0);
            mConeActor->SetUserTransform(transform);
        } else if (key == "Right") {
            transform->SetInput(userTransform);
            transform->Translate(1, 0, 0);
            mConeActor->SetUserTransform(transform);
        }
        mRenderWindow->Render();

        vtkInteractorStyleTrackballCamera::OnKeyPress();
    }
void SetConeActor(vtkActor *coneActor) { mConeActor = coneActor; }

void SetRenderWindow(vtkRenderWindow *window) { mRenderWindow = window; }

private:
    vtkActor *mConeActor;
    vtkRenderWindow *mRenderWindow;
};
vtkStandardNewMacro(KeyPressInteractorStyle);

int main(int, char *[]) {
    vtkNew<vtkNamedColors> colors;

    vtkNew<vtkConeSource> cone;
    cone->SetHeight(3.0);
    cone->SetRadius(1.0);
    cone->SetResolution(10);

    vtkNew<vtkPolyDataMapper> coneMapper;
    coneMapper->SetInputConnection(cone->GetOutputPort());

    vtkNew<vtkActor> coneActor;
    coneActor->SetMapper(coneMapper);
    coneActor->GetProperty()->SetColor(colors->GetColor3d("Bisque").GetData());

    vtkNew<vtkRenderer> renderer;
    renderer->AddActor(coneActor);
    renderer->SetBackground(colors->GetColor3d("MidnightBlue").GetData());

    vtkNew<vtkRenderWindow> renderWindow;
    renderWindow->AddRenderer(renderer);
    renderWindow->SetSize(800, 800);
    renderWindow->SetWindowName("MoveActorByKeyboard");

    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(renderWindow);

    vtkNew<KeyPressInteractorStyle> style;
    style->SetConeActor(coneActor);
    style->SetRenderWindow(renderWindow);
    interactor->SetInteractorStyle(style);

    interactor->Initialize();
    interactor->Start();

    return EXIT_SUCCESS;
}
