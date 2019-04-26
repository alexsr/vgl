cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

set(project_name vgl)

include(cmake/macros.cmake)

set(vgl_major_version 0)
set(vgl_minor_version 1)
set(vgl_patch_version 0)

project(vgl
			VERSION ${vgl_major_version}.${vgl_minor_version}.${vgl_patch_version}
			DESCRIPTION "Personal CG and CV project"
			LANGUAGES C CXX)

option(vgl_build_tests OFF)
option(vgl_install OFF)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)
set(EXTERNAL_PATH ${CMAKE_SOURCE_DIR}/external)
set(SRC_PATH ${CMAKE_SOURCE_DIR}/src)

set(LIB_PATH ${PROJECT_BINARY_DIR}/lib)
set(BIN_PATH ${PROJECT_BINARY_DIR}/bin)
set(SHADERS_BIN_PATH ${PROJECT_BINARY_DIR}/shaders)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIB_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${LIB_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${LIB_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_PATH})

include(cmake/external.cmake)

add_subdirectory(src)
add_subdirectory(projects)
add_subdirectory(shaders)
