cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

add_executable(demo demo/main.cpp)
target_link_libraries(demo PUBLIC OpenGL::GL glad::glad glfw::glfw assimp::assimp
							stb::stb imgui::imgui glm::glm nativefiledialog vgl)
