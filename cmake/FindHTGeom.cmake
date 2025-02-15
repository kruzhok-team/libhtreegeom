# Locate libhtgeom library
# This module defines
#  HTGeom_FOUND, if false, do not try to link to QtPropertyBrowser
#  HTGeom_LIBRARY
#  HTGeom_INCLUDE_DIR, where to find qtpropertybrowser.h
#  HTGeom_DIR - Can be set to HTGeom install path or Windows build path

find_path(HTGeom_INCLUDE_DIR htgeom.h
  HINTS ${HTGeom_DIR}
  PATH_SUFFIXES include cyberiada
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
)

find_library(HTGeom_LIBRARY
  NAMES htgeom
  HINTS ${HTGeom_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set HTGeom_FOUND to TRUE if 
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HTGeom DEFAULT_MSG HTGeom_LIBRARY HTGeom_INCLUDE_DIR)

set (HTGeom_LIBRARIES htgeom)
mark_as_advanced(HTGeom_INCLUDE_DIR HTGeom_LIBRARY)
