#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "vtkSurface" for configuration "Release"
set_property(TARGET vtkSurface APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(vtkSurface PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "C:/Program Files/ACVD/lib/vtkSurface.lib"
  )

list(APPEND _cmake_import_check_targets vtkSurface )
list(APPEND _cmake_import_check_files_for_vtkSurface "C:/Program Files/ACVD/lib/vtkSurface.lib" )

# Import target "vtkVolumeProcessing" for configuration "Release"
set_property(TARGET vtkVolumeProcessing APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(vtkVolumeProcessing PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "C:/Program Files/ACVD/lib/vtkVolumeProcessing.lib"
  )

list(APPEND _cmake_import_check_targets vtkVolumeProcessing )
list(APPEND _cmake_import_check_files_for_vtkVolumeProcessing "C:/Program Files/ACVD/lib/vtkVolumeProcessing.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
