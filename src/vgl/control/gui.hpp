#pragma once

#include <array>
#include <vgl/gpu_api/gl/handle.hpp>
#include <imgui/imgui.h>

struct GLFWwindow;
struct GLFWcursor;

namespace vgl {
    class Window;
    namespace ui {
        class Gui {
        public:
            explicit Gui(Window& window, float font_size = 18.0f, bool install_callbacks_flag = true);
            Gui(const Gui&) = delete;
            Gui& operator=(const Gui&) = delete;
            Gui(Gui&&) = default;
            Gui& operator=(Gui&&) = default;
            ~Gui();
            void start_frame();
            void render();
            void setup_fonts(float font_size);
        private:
            gl::glprogram _program;
            gl::gltexture _font_texture;
            gl::glvertexarray _vao_handle{};
            gl::glbuffer _vbo_handle;
            gl::glbuffer _ebo_handle;
            std::array<GLFWcursor*, ImGuiMouseCursor_COUNT> _mouse_cursors{};
            double _time;
            GLFWwindow* _window_ptr;
        };
    }
}

namespace ImGui
{
    void StyleColorsVS(ImGuiStyle* dst = nullptr);
    /*ImTextureID ConvertGLuintToImTex(const GLuint t_id) {
        return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(t_id));
    }*/
}
