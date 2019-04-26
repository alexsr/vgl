cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

add_library(glad ${EXTERNAL_PATH}/glad/src/glad.c)
add_library(glad::glad ALIAS glad)
target_include_directories(glad
    PUBLIC ${EXTERNAL_PATH}/glad/include/
)
target_compile_features(glad INTERFACE cxx_std_17)
set_target_properties(glad PROPERTIES
    LINKER_LANGUAGE CXX
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
