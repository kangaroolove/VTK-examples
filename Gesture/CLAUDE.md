# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a VTK-Qt integration example application that demonstrates how to embed VTK rendering within a Qt widget. The application creates a simple 3D visualization using VTK's OpenGL rendering capabilities within a Qt interface.

## Build System

The project uses CMake as its build system and is configured for Visual Studio on Windows.

### Build Commands

**Configure and generate build files:**
```bash
cd build
cmake ..
```

**Build the application:**
```bash
cmake --build . --config Debug
```
Or open `GenericQtApplication.sln` in Visual Studio and build from the IDE.

**Run the application:**
```bash
./build/Debug/GenericQtApplication.exe
```

### Dependencies

- **VTK**: Used for 3D visualization and rendering
  - Required VTK components: vtkCommonCore, vtkViewsQt, vtkIOGeometry, vtkInteractionStyle
  - Must be built with Qt support (VTK_QT_VERSION must be set)
- **Qt5**: Used for the GUI framework
  - Required Qt components: Widgets
- **OpenGL**: For hardware-accelerated rendering

## Architecture

The application follows a simple architecture with three main components:

### Core Classes

1. **VTKOpenGLWidget** (`VTKOpenGLWidget.h/.cpp`):
   - Inherits from `QVTKOpenGLNativeWidget` 
   - Manages VTK render window and renderer
   - Handles OpenGL context integration between Qt and VTK
   - Contains test data creation for demonstration

2. **Main Application** (`main.cpp`):
   - Sets up proper OpenGL surface format for VTK rendering
   - Creates and displays the main widget

### Key Integration Points

- **OpenGL Context Setup**: The application uses `QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat())` to ensure proper OpenGL context creation for VTK rendering.

- **VTK-Qt Bridge**: Uses `QVTKOpenGLNativeWidget` as the base class to integrate VTK's OpenGL rendering with Qt's widget system.

- **Render Pipeline**: The `VTKOpenGLWidget` class manages a complete VTK rendering pipeline:
  - `vtkGenericOpenGLRenderWindow`: The render window
  - `vtkRenderer`: The renderer that manages actors and cameras
  - Test data creation using `vtkConeSource` for demonstration

## Development Workflow

When making changes to this codebase:

1. **Modifying VTK rendering**: Edit the `createTestData()` method in `VTKOpenGLWidget.cpp` to change what is rendered
2. **Adding new VTK components**: Update `CMakeLists.txt` to include additional VTK modules in the `find_package(VTK COMPONENTS ...)` section
3. **Qt UI changes**: The current implementation has minimal Qt UI - extend the widget or create additional Qt components as needed

## File Structure

- `main.cpp`: Application entry point and OpenGL setup
- `VTKOpenGLWidget.h/.cpp`: Main widget integrating VTK and Qt
- `CMakeLists.txt`: Build configuration
- `build/`: Generated build files and executable output