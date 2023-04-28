
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(MoveActorByKeyboard)

find_package(VTK COMPONENTS 
  vtkCommonColor
  vtkCommonCore
  vtkFiltersSources
  vtkInteractionStyle
  vtkRenderingContextOpenGL2
  vtkRenderingCore
  vtkRenderingFreeType
  vtkRenderingGL2PSOpenGL2
  vtkRenderingOpenGL2
)

include(${VTK_USE_FILE})

if (NOT VTK_FOUND)
  message(FATAL_ERROR "Tutorial_Step5: Unable to find the VTK build folder.")
endif()

# Prevent a "command line is too long" failure in Windows.
set(CMAKE_NINJA_FORCE_RESPONSE_FILE "ON" CACHE BOOL "Force Ninja to use response files.")
add_executable(MoveActorByKeyboard MACOSX_BUNDLE main.cxx )
  target_link_libraries(MoveActorByKeyboard PRIVATE ${VTK_LIBRARIES}
)
# vtk_module_autoinit is needed
#vtk_module_autoinit(
#  TARGETS Tutorial_Step5
#  MODULES ${VTK_LIBRARIES}
#)
