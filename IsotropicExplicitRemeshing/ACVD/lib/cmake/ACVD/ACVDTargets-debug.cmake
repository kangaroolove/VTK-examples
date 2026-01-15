#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "vtkSurface" for configuration "Debug"
set_property(TARGET vtkSurface APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(vtkSurface PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "C:/Program Files/ACVD/lib/vtkSurface-d.lib"
  )

list(APPEND _cmake_import_check_targets vtkSurface )
list(APPEND _cmake_import_check_files_for_vtkSurface "C:/Program Files/ACVD/lib/vtkSurface-d.lib" )

# Import target "vtkVolumeProcessing" for configuration "Debug"
set_property(TARGET vtkVolumeProcessing APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(vtkVolumeProcessing PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "C:/Program Files/ACVD/lib/vtkVolumeProcessing-d.lib"
  )

list(APPEND _cmake_import_check_targets vtkVolumeProcessing )
list(APPEND _cmake_import_check_files_for_vtkVolumeProcessing "C:/Program Files/ACVD/lib/vtkVolumeProcessing-d.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
