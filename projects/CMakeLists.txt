cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

add_executable(textured_model textured_model/main.cpp)
target_link_libraries(textured_model PUBLIC vgl)

add_executable(normal_mapping normal_mapping/main.cpp normal_mapping/demo_util.hpp)
target_link_libraries(normal_mapping PUBLIC vgl)

add_executable(pbr pbr/main.cpp pbr/demo_util.hpp)
target_link_libraries(pbr PUBLIC vgl)

add_executable(raytracing raytracing/main.cpp)
target_link_libraries(raytracing PUBLIC vgl)

add_executable(deferred deferred/main.cpp)
target_link_libraries(deferred PUBLIC vgl)

add_executable(cubemap_generator cubemap_generator/main.cpp)
target_link_libraries(cubemap_generator PUBLIC stb::stb)

add_executable(music_vis music_vis/main.cpp music_vis/audio_input.hpp)
target_link_libraries(music_vis PUBLIC vgl portaudio)

add_subdirectory(computer_vision)
