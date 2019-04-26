add_library(imgui STATIC
    ${EXTERNAL_PATH}/imgui/imgui/imgui.cpp
    ${EXTERNAL_PATH}/imgui/imgui/imgui_demo.cpp
    ${EXTERNAL_PATH}/imgui/imgui/imgui_draw.cpp
    ${EXTERNAL_PATH}/imgui/imgui/imgui_widgets.cpp
    ${EXTERNAL_PATH}/imgui/imgui/misc/cpp/imgui_stdlib.cpp
    ${EXTERNAL_PATH}/imgui/imgui/examples/imgui_impl_glfw.cpp
    ${EXTERNAL_PATH}/imgui/imgui/examples/imgui_impl_opengl3.cpp
	)
    
target_include_directories(imgui INTERFACE ${EXTERNAL_PATH}/imgui/)
target_include_directories(imgui PRIVATE ${EXTERNAL_PATH}/imgui/imgui/)

add_library(imgui::imgui ALIAS imgui)

target_link_libraries(imgui PRIVATE glad::glad glfw::glfw)

target_compile_features(imgui INTERFACE cxx_std_17)
target_compile_definitions(imgui
    PUBLIC 
        IMGUI_IMPL_OPENGL_LOADER_GLAD)
