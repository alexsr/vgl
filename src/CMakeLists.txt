cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

configure_file(${SRC_PATH}/vgl/file/file_paths.hpp.in ${SRC_PATH}/vgl/file/file_paths.hpp @ONLY)

add_library(vgl INTERFACE)
target_include_directories(vgl
	INTERFACE ${SRC_PATH})

target_compile_definitions(vgl
    INTERFACE 
        GLFW_INCLUDE_NONE
        GLM_FORCE_RADIANS
        GLM_ENABLE_EXPERIMENTAL)

target_compile_features(vgl INTERFACE cxx_std_17)
target_link_libraries(vgl INTERFACE OpenGL::GL glad::glad glfw::glfw assimp::assimp
								  stb::stb imgui::imgui glm::glm nativefiledialog jbraun::glsp)
