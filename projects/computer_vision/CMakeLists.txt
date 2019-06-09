cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

add_executable(example_filtered example_filtered/main.cpp)
target_link_libraries(example_filtered PUBLIC vgl)

add_executable(example_separated_filters example_separated_filters/main.cpp)
target_link_libraries(example_separated_filters PUBLIC vgl)

add_executable(laplacian_of_gaussian laplacian_of_gaussian/main.cpp)
target_link_libraries(laplacian_of_gaussian PUBLIC vgl)

add_executable(difference_of_gaussian difference_of_gaussian/main.cpp)
target_link_libraries(difference_of_gaussian PUBLIC vgl)

add_executable(moravec moravec/main.cpp)
target_link_libraries(moravec PUBLIC vgl)

add_executable(harris harris/main.cpp)
target_link_libraries(harris PUBLIC vgl)

add_executable(shi_tomasi shi_tomasi/main.cpp)
target_link_libraries(shi_tomasi PUBLIC vgl)

add_executable(canny canny/main.cpp)
target_link_libraries(canny PUBLIC vgl)

add_executable(rot13 rot13/main.cpp)
target_link_libraries(rot13 PUBLIC vgl)
