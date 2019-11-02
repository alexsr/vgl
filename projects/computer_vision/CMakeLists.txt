cmake_minimum_required(VERSION 3.5...3.15)

if(${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

include(${CMAKE_SOURCE_DIR}/cmake/projects.cmake)

add_vgl_exe(example_filtered libvgl)
add_vgl_exe(example_separated_filters libvgl)
add_vgl_exe(laplacian_of_gaussian libvgl)
add_vgl_exe(difference_of_gaussian libvgl)
add_vgl_exe(moravec libvgl)
add_vgl_exe(harris libvgl)
add_vgl_exe(shi_tomasi libvgl)
add_vgl_exe(canny libvgl)
add_vgl_exe(rot13 libvgl)
