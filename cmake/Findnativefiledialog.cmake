cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

set(NFD_SOURCE_PATH ${EXTERNAL_PATH}/nativefiledialog/src)

set(NFD_SOURCES ${NFD_SOURCE_PATH}/nfd_common.c)

# add specific implementations
if (WIN32)
    list(APPEND NFD_SOURCES ${NFD_SOURCE_PATH}/nfd_win.cpp)
elseif (APPLE)
    list(APPEND NFD_SOURCES ${NFD_SOURCE_PATH}/nfd_cocoa.m)
elseif (UNIX)
    list(APPEND NFD_SOURCES ${NFD_SOURCE_PATH}/nfd_gtk.c)
endif ()

add_library(nativefiledialog STATIC ${NFD_SOURCES})

target_include_directories(nativefiledialog PUBLIC ${NFD_SOURCE_PATH}/include)
