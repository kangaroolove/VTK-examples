# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a minimal Qt5 + VTK (Visualization Toolkit) integration example that demonstrates 3D rendering in a Qt widget using OpenGL. The application creates a Qt window with an embedded VTK rendering widget displaying a simple 3D cone.

## Build System

This project uses CMake with Visual Studio generators on Windows.

### Build Configuration

The project requires specific paths to Qt5 and VTK libraries configured via CMake arguments:

```bash
cmake -DQt5_DIR=D:/Qt/Qt5.12.10/5.12.10/msvc2015_64/lib/cmake/Qt5 -DVTK_DIR=D:/work/libs/VTK/x64/lib/cmake/vtk-8.2 ..
```

**Note:** The paths above are environment-specific. Update them to match your local Qt5 and VTK installation paths.

### Building the Project

```bash
# Configure (from build directory)
cmake -DQt5_DIR=<path-to-qt5-cmake> -DVTK_DIR=<path-to-vtk-cmake> ..

# Build
cmake --build . --config Debug
# or
cmake --build . --config Release

# Using Visual Studio directly
# Open build/GenericQtApplication.sln in Visual Studio and build
```

### Running the Application

```bash
# From build directory
Debug/GenericQtApplication.exe
# or
Release/GenericQtApplication.exe
```

## Architecture

### Core Components

1. **VTKOpenGLWidget** (`VTKOpenGLWidget.h/cpp`)
   - Inherits from `QVTKOpenGLNativeWidget` (Qt's VTK-OpenGL integration widget)
   - Manages VTK render window and renderer lifecycle
   - Sets up the VTK visualization pipeline
   - Pattern: Widget encapsulates both Qt widget functionality and VTK rendering setup

2. **main.cpp**
   - Application entry point
   - **Critical:** Must call `QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat())` before creating QApplication to ensure proper OpenGL context for VTK

### VTK Pipeline Architecture

The VTK pipeline follows the standard source-mapper-actor-renderer pattern:
- **Source** (e.g., `vtkConeSource`): Generates geometry data
- **Mapper** (e.g., `vtkPolyDataMapper`): Converts geometry to renderable primitives
- **Actor** (`vtkActor`): Represents visual object in scene with properties
- **Renderer** (`vtkRenderer`): Manages scene and camera
- **RenderWindow** (`vtkGenericOpenGLRenderWindow`): Handles OpenGL context

### Memory Management

VTK uses reference-counted smart pointers:
- `vtkSmartPointer<T>`: For member variables and long-lived objects
- `vtkNew<T>`: For local/temporary objects, lighter weight

## CMake Configuration Details

- **CMAKE_AUTOMOC/AUTOUIC/AUTORCC**: Enabled for automatic Qt meta-object compilation
- **CMAKE_CXX_STANDARD**: C++11 (minimum required)
- **VTK Components Required**:
  - vtkCommonCore
  - vtkViewsQt
  - vtkIOGeometry
  - vtkInteractionStyle
- **Qt Components Required**: Qt5::Widgets

## Development Notes

### Adding New VTK Visualizations

To add new 3D visualizations, follow the pattern in `VTKOpenGLWidget::createTestData()`:
1. Create VTK source (geometry generator)
2. Create mapper and connect to source
3. Create actor and connect to mapper
4. Add actor to renderer using `m_renderer->AddActor()`

### Qt-VTK Integration

- Always use `QVTKOpenGLNativeWidget` as the base class (not older `QVTKWidget`)
- Set default OpenGL surface format before QApplication instantiation
- The render window must be of type `vtkGenericOpenGLRenderWindow` for Qt integration
- Use `SetRenderWindow()` to connect VTK render window to Qt widget

### Debugging Configuration

The `.vscode/settings.json` includes a Qt visualizer file reference for better debugging experience in Visual Studio. Update the path to your local Qt natvis file if needed.
