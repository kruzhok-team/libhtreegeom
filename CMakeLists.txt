cmake_minimum_required(VERSION 3.16)

project(htgeom VERSION 1.0.6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

if (WIN32)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/homog2d.hpp)
  message(FATAL_ERROR "Cannot find homog2d.hpp (download here: https://github.com/skramm/homog2d)")
endif()

add_library(htgeom SHARED htgeom_types.cpp htgeom.cpp)
target_include_directories(htgeom PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/cyberiada>)

if(MSVC)
  add_compile_options(/W4)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall)
  add_compile_options(-Wextra)
  add_compile_options(-Wpedantic)
endif()
add_compile_definitions($<$<CONFIG:Debug>:__DEBUG__>)

# homog2d.hpp (bundled third-party) has a dead getBB() fallback that references a
# non-existent member; GCC 15+ checks template bodies eagerly (-Wtemplate-body) and
# rejects it although it is never instantiated. Silence the check where the compiler
# has the flag (older compilers lack it and do not perform the eager check).
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-Wtemplate-body" HTGEOM_HAVE_WTEMPLATE_BODY)
if(HTGEOM_HAVE_WTEMPLATE_BODY)
  target_compile_options(htgeom PRIVATE -Wno-template-body)
endif()

if (NOT WIN32)
   target_link_libraries(htgeom PUBLIC)
endif()

add_executable(htgeom_test htgeom_test.cpp)
target_include_directories(htgeom_test PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/cyberiada>)
target_link_directories(htgeom_test PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(htgeom_test PUBLIC htgeom)

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/tests/")
file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
foreach(source_path ${files})
  get_filename_component(target_name "${source_path}" NAME_WE)
  string(CONCAT target_name "${target_name}" ".test")
  add_executable("${target_name}" "${source_path}")
  target_include_directories("${target_name}" PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
  target_link_directories("${target_name}" PUBLIC "${PROJECT_BINARY_DIR}")
  target_link_libraries("${target_name}" PUBLIC htgeom)
  add_custom_command(TARGET "${target_name}"
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${target_name}>" "${PROJECT_BINARY_DIR}/tests/"
    )
  string(REGEX MATCH "^[0-9]+" test_num "${target_name}")
  set(expected_file "${CMAKE_CURRENT_SOURCE_DIR}/tests/${test_num}-output.txt")
  if (EXISTS "${expected_file}")
    add_test(NAME "${target_name}"
      COMMAND "${CMAKE_COMMAND}"
        -DTEST_PROG=$<TARGET_FILE:${target_name}>
        -DTEST_OUTPUT=${PROJECT_BINARY_DIR}/tests/${target_name}.txt
        -DTEST_EXPECTED=${expected_file}
        -DTEST_EMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunDiffTest.cmake)
  else()
    add_test(NAME "${target_name}" COMMAND "${target_name}")
  endif()
endforeach()

file(GLOB test_files
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.graphml"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.txt"
  )
foreach(f ${test_files})
  file(COPY "${f}" DESTINATION "${PROJECT_BINARY_DIR}/tests/")
endforeach()

file(COPY "${PROJECT_SOURCE_DIR}/run-tests.sh" DESTINATION "${PROJECT_BINARY_DIR}")

install(TARGETS htgeom DESTINATION lib EXPORT htgeom
	      COMPONENT runtime)
install(FILES htgeom.h
    	      DESTINATION include/cyberiada
	      COMPONENT dev)	
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindHTGeom.cmake
  	      DESTINATION lib/cmake
	      COMPONENT dev)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/htgeom-config.cmake
  	      DESTINATION lib/cmake/htgeom
	      COMPONENT dev)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/docs/architecture.md
	      ${CMAKE_CURRENT_SOURCE_DIR}/docs/api.md
	      DESTINATION share/doc/libhtreegeom
	      COMPONENT dev)

configure_file ("${PROJECT_SOURCE_DIR}/HTreeCPackOptions.cmake.in"
                "${PROJECT_BINARY_DIR}/HTreeCPackOptions.cmake"
                @ONLY)
set (CPACK_PROJECT_CONFIG_FILE
     "${PROJECT_BINARY_DIR}/HTreeCPackOptions.cmake")

include(CPack)
