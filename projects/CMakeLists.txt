cmake_minimum_required(VERSION 3.5...3.15)

if(${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

include(${CMAKE_SOURCE_DIR}/cmake/projects.cmake)

add_vgl_exe(textured_model libvgl)
add_vgl_exe(normal_mapping libvgl)
add_vgl_exe(pbr libvgl)
add_vgl_exe(raytracing libvgl)
add_vgl_exe(deferred libvgl)
add_vgl_exe(raytracing_cpu libvgl)

add_executable(cubemap_generator cubemap_generator/main.cpp)
target_link_libraries(cubemap_generator PUBLIC stb::stb)
target_compile_features(cubemap_generator PUBLIC cxx_std_17)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set_target_properties(cubemap_generator PROPERTIES
			LINK_FLAGS "/NODEFAULTLIB:MSVCRT")
endif()

add_custom_command(
        TARGET textured_model POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/../cppdependencies/install-x64-Release/bin/
                ${CMAKE_CURRENT_BINARY_DIR}/)

add_subdirectory(computer_vision)
