cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

find_path(PORTAUDIO_INCLUDE_PATH
    NAMES portaudio/portaudio.h
    PATHS ${EXTERNAL_PATH}/portaudio
    PATH_SUFFIXES include)

find_library(PORTAUDIO_LIB_PATH
    NAMES portaudio_static_x64.lib
    PATHS ${EXTERNAL_PATH}/portaudio
    PATH_SUFFIXES lib)

add_library(portaudio INTERFACE IMPORTED)
target_link_libraries(portaudio INTERFACE ${PORTAUDIO_LIB_PATH})
target_include_directories(portaudio INTERFACE ${PORTAUDIO_INCLUDE_PATH}
)
