#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "oops::security_engine" for configuration "Debug"
set_property(TARGET oops::security_engine APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(oops::security_engine PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libsecurity_engine.a"
  )

list(APPEND _cmake_import_check_targets oops::security_engine )
list(APPEND _cmake_import_check_files_for_oops::security_engine "${_IMPORT_PREFIX}/lib/libsecurity_engine.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
