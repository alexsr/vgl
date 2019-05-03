cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

add_executable(example_model example_model/main.cpp)
target_link_libraries(example_model PUBLIC OpenGL::GL glad::glad glfw::glfw assimp::assimp
							stb::stb imgui::imgui glm::glm nativefiledialog vgl)

add_executable(example_texture example_texture/main.cpp)
target_link_libraries(example_texture PUBLIC OpenGL::GL glad::glad glfw::glfw assimp::assimp
							stb::stb imgui::imgui glm::glm nativefiledialog vgl)

add_executable(example_filtered example_filtered/main.cpp)
target_link_libraries(example_filtered PUBLIC OpenGL::GL glad::glad glfw::glfw assimp::assimp
							stb::stb imgui::imgui glm::glm nativefiledialog vgl)

add_executable(example_separated_filters example_separated_filters/main.cpp)
target_link_libraries(example_separated_filters PUBLIC OpenGL::GL glad::glad glfw::glfw assimp::assimp
							stb::stb imgui::imgui glm::glm nativefiledialog vgl)

add_executable(laplacian_of_gaussian laplacian_of_gaussian/main.cpp)
target_link_libraries(laplacian_of_gaussian PUBLIC OpenGL::GL glad::glad glfw::glfw assimp::assimp
							stb::stb imgui::imgui glm::glm nativefiledialog vgl)

add_executable(difference_of_gaussian difference_of_gaussian/main.cpp)
target_link_libraries(difference_of_gaussian PUBLIC OpenGL::GL glad::glad glfw::glfw assimp::assimp
							stb::stb imgui::imgui glm::glm nativefiledialog vgl)

add_executable(moravec moravec/main.cpp)
target_link_libraries(moravec PUBLIC OpenGL::GL glad::glad glfw::glfw assimp::assimp
							stb::stb imgui::imgui glm::glm nativefiledialog vgl)

add_executable(harris harris/main.cpp)
target_link_libraries(harris PUBLIC OpenGL::GL glad::glad glfw::glfw assimp::assimp
							stb::stb imgui::imgui glm::glm nativefiledialog vgl)

add_executable(shi_tomasi shi_tomasi/main.cpp)
target_link_libraries(shi_tomasi PUBLIC OpenGL::GL glad::glad glfw::glfw assimp::assimp
							stb::stb imgui::imgui glm::glm nativefiledialog vgl)

add_executable(canny canny/main.cpp)
target_link_libraries(canny PUBLIC OpenGL::GL glad::glad glfw::glfw assimp::assimp
							stb::stb imgui::imgui glm::glm nativefiledialog vgl)
