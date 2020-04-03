#pragma once

#include <array>
#include "vgl/gpu_api/gl/handle.hpp"
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
            gl::glprogram program_;
            gl::gltexture font_texture_;
            gl::glvertexarray vao_handle_{};
            gl::glbuffer vbo_handle_;
            gl::glbuffer ebo_handle_;
            std::array<GLFWcursor*, ImGuiMouseCursor_COUNT> mouse_cursors_{};
            double time_;
            GLFWwindow* window_ptr_;
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
