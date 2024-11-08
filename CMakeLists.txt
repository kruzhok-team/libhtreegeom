cmake_minimum_required(VERSION 3.21)

project(htgeom VERSION 1.0)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D__DEBUG__")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/homog2d.hpp)
   	message(FATAL_ERROR "Cannot find homog2d.hpp (download here: https://github.com/skramm/homog2d)")
endif()

add_library(htgeom SHARED htgeom_types.cpp htgeom.cpp)
target_include_directories(htgeom PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/cyberiada>)
			 
target_link_libraries(htgeom PUBLIC)

add_executable(htgeom_test htgeom_test.cpp)
target_include_directories(htgeom_test PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/cyberiada>)
target_link_directories(htgeom_test PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(htgeom_test PUBLIC htgeom)

install(TARGETS htgeom DESTINATION lib EXPORT htgeom)
install(FILES htgeom.h
  ${CMAKE_CURRENT_SOURCE_DIR}/htgeom.h
  DESTINATION include/cyberiada)
install(EXPORT htgeom DESTINATION lib/cmake)

configure_file(
  ${PROJECT_SOURCE_DIR}/htgeom-config.cmake.in
  ${PROJECT_BINARY_DIR}/htgeom-config.cmake @ONLY)

install(FILES
  ${PROJECT_BINARY_DIR}/htgeom-config.cmake
  DESTINATION lib/cmake)
