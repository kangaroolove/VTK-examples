cmake_minimum_required(VERSION 3.5)

project(Marker VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(VTK COMPONENTS   
  vtkCommonCore
  vtkViewsQt
  vtkIOGeometry
  vtkInteractionStyle
  vtkRenderingAnnotation
)

include(${VTK_USE_FILE})
if("${VTK_QT_VERSION}" STREQUAL "")
  message(FATAL_ERROR "VTK was not built with Qt")
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)

include_directories(
)

list(APPEND EXTRA_LIBS 
)

set(PROJECT_SOURCES
    main.cpp
    VTKOpenGLWidget.cpp
    VTKOpenGLWidget.h
    MarkerAssembly.cpp
    MarkerAssembly.h
    Widget.h
    Widget.cpp
    MarkerActor.cpp
    MarkerActor.h
)

set(PROJECT_UIS
)

set(PROJECT_RESOURCES
)

add_executable(${PROJECT_NAME} 
	${PROJECT_SOURCES}
	${PROJECT_UIS}
	${PROJECT_RESOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets ${VTK_LIBRARIES} ${EXTRA_LIBS})