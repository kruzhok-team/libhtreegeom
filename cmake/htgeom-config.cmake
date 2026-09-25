# The package configuration of libhtgeom: find_package(HTGeom) locates it
# through CMAKE_PREFIX_PATH or the system prefix, no module path needed.
# It defines the same variables as FindHTGeom.cmake:
#  HTGeom_FOUND, HTGeom_INCLUDE_DIR, HTGeom_LIBRARY, HTGeom_LIBRARIES

get_filename_component(_htgeom_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

# search this installation only, so a stale copy elsewhere cannot win
find_path(HTGeom_INCLUDE_DIR htgeom.h
  PATHS "${_htgeom_prefix}/include/cyberiada" NO_DEFAULT_PATH)
find_library(HTGeom_LIBRARY NAMES htgeom
  PATHS "${_htgeom_prefix}/lib" NO_DEFAULT_PATH)
mark_as_advanced(HTGeom_INCLUDE_DIR HTGeom_LIBRARY)

if(HTGeom_INCLUDE_DIR AND HTGeom_LIBRARY)
  set(HTGeom_LIBRARIES htgeom)
else()
  set(HTGeom_FOUND FALSE)
  set(HTGeom_NOT_FOUND_MESSAGE "htgeom.h or the htgeom library is missing under ${_htgeom_prefix}")
endif()

unset(_htgeom_prefix)
