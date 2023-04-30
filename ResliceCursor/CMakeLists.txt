
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(ResliceCursor)

find_package(VTK COMPONENTS 
  vtkCommonColor
  vtkCommonCore
  vtkCommonDataModel
  vtkFiltersGeneral
  vtkFiltersSources
  vtkInteractionStyle
  vtkRenderingContextOpenGL2
  vtkRenderingCore
  vtkRenderingFreeType
  vtkRenderingGL2PSOpenGL2
  vtkRenderingOpenGL2
  vtkInteractionWidgets
  vtkRenderingImage
)

include(${VTK_USE_FILE})

if (NOT VTK_FOUND)
  message(FATAL_ERROR "Cursor2D: Unable to find the VTK build folder.")
endif()

# Prevent a "command line is too long" failure in Windows.
set(CMAKE_NINJA_FORCE_RESPONSE_FILE "ON" CACHE BOOL "Force Ninja to use response files.")
add_executable(ResliceCursor MACOSX_BUNDLE ResliceCursor.cxx )
  target_link_libraries(ResliceCursor PRIVATE ${VTK_LIBRARIES}
)

# vtk_module_autoinit is needed
#vtk_module_autoinit(
#  TARGETS Cursor2D
#  MODULES ${VTK_LIBRARIES}
#)
