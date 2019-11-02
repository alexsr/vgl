function(add_vgl_exe EXENAME LIBS)
    file(GLOB SRC_FILES
        ${EXENAME}/*.h
		${EXENAME}/*.cpp)
    add_executable(${EXENAME} ${SRC_FILES})
    target_link_libraries(${EXENAME} PUBLIC ${LIBS})
    target_compile_features(${EXENAME} PUBLIC cxx_std_17)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(${EXENAME} PROPERTIES
                LINK_FLAGS "/NODEFAULTLIB:MSVCRT")
    endif()
endfunction()
