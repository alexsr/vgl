cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

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

option(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)
set(EXTERNAL_PATH ${CMAKE_SOURCE_DIR}/external)
set(SRC_PATH ${CMAKE_SOURCE_DIR}/src)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(glsp_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cppdependencies/install-x64-Debug/lib/cmake/glsp/)
endif()
if (CMAKE_BUILD_TYPE STREQUAL "Release")
	set(glsp_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cppdependencies/install-x64-Release/lib/cmake/glsp/)
endif()

find_package(OpenMP REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glad REQUIRED)
find_package(glm REQUIRED)
find_package(stb REQUIRED)
find_package(glfw3 REQUIRED)
find_package(imgui REQUIRED)
find_package(glsp REQUIRED)
find_package(iconfont REQUIRED)
find_package(nativefiledialog REQUIRED)
find_package(assimp REQUIRED)

add_subdirectory(src)
add_subdirectory(projects)
