cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

configure_file(${SRC_PATH}/vgl/file/file_paths.hpp.in ${SRC_PATH}/vgl/file/file_paths.hpp @ONLY)

add_library(libvgl
    vgl/file/stb_impl.cpp
    vgl/file/file.cpp
    vgl/file/image_file.cpp
    vgl/file/image.cpp
    vgl/gpu_api/gl/handle.cpp
    vgl/rendering/camera.cpp
    vgl/rendering/scene.cpp
    vgl/core/window.cpp
    vgl/core/gui.cpp
    vgl/math/pose.cpp)

target_include_directories(libvgl
	PUBLIC
		$<BUILD_INTERFACE:${SRC_PATH}>)


target_compile_definitions(libvgl
    PUBLIC 
        GLFW_INCLUDE_NONE
        GLM_FORCE_RADIANS
        GLM_ENABLE_EXPERIMENTAL)

target_compile_features(libvgl PUBLIC cxx_std_17)
set_target_properties(libvgl PROPERTIES
            LINKER_LANGUAGE CXX
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            CXX_STANDARD_REQUIRED ON)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(libvgl PROPERTIES
            LINK_FLAGS "/NODEFAULTLIB:MSVCRT")
endif()

target_link_libraries(libvgl PUBLIC OpenGL::GL glad::glad glfw assimp::assimp stb::stb OpenMP::OpenMP_CXX
                      imgui::imgui Eigen3::Eigen nativefiledialog::nativefiledialog glsp::glsp iconfont::iconfont)
