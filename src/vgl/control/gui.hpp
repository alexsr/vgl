#pragma once

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <array>
#include <cassert>
#include <utility>
#include <iconfont/IconsMaterialDesignIcons.h>
#include "window.hpp"
#include <vgl/gpu_api/gl/handle.hpp>
#include "vgl/file/file_paths.hpp"
#include "vgl/file/image_file.hpp"
#include "vgl/gpu_api/gl/multidraw.hpp"
#include "vgl/gpu_api/gl/texture.hpp"

namespace vgl::ui
{
    namespace impl {
        constexpr const char* imgui_vs = R"(#version 460

layout (location = 0) uniform mat4 ProjMtx;
in vec2 Position;
in vec2 UV;
in vec4 Color;
out vec2 Frag_UV;
out vec4 Frag_Color;
out flat int draw_id;

void main() {
    draw_id = gl_DrawID;
    Frag_UV = UV;
    Frag_Color = Color;
    gl_Position = ProjMtx * vec4(Position.xy,0,1);
})";
        constexpr const char* imgui_fs = R"(#version 460

#extension GL_ARB_bindless_texture : require

in vec2 Frag_UV;
in vec4 Frag_Color;
in flat int draw_id;
out vec4 Out_Color;

layout (std430, binding = 0) buffer tex_ref_buffer {
    sampler2D textures[];
};

void main() {
    Out_Color = Frag_Color * texture(textures[draw_id], Frag_UV.st);
})";
    }

    class Gui {
    public:
        explicit Gui(window& window, float font_size = 18.0f, bool install_callbacks_flag = true) {
            _window_ptr = window.get();
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            auto& io = ImGui::GetIO();
            auto vs = gl::create_shader(GL_VERTEX_SHADER, impl::imgui_vs, strlen(impl::imgui_vs));
            auto fs = gl::create_shader(GL_FRAGMENT_SHADER, impl::imgui_fs, strlen(impl::imgui_fs));
            _program = gl::create_program({ vs, fs });
            const auto window_size = window.size();
            const auto framebuffer_size = window.framebuffer_size();

            setup_fonts(font_size);

            // Setup back-end capabilities flags
            io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values (optional)
            io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
            // We can honor io.WantSetMousePos requests (optional, rarely used)

            // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
            io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
            io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
            io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
            io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
            io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
            io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
            io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
            io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
            io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
            io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
            io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
            io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
            io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
            io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
            io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
            io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
            io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
            io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
            io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
            io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
            io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

            // Cursor setup
            _mouse_cursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
            _mouse_cursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
            _mouse_cursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
            _mouse_cursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
            _mouse_cursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
            _mouse_cursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
            _mouse_cursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
            _mouse_cursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

            auto set_clipboard_string = [](void* u, const char* c) {
                glfwSetClipboardString((GLFWwindow*)u, c);
            };
            auto get_clipboard_string = [](void* u) -> auto {
                return glfwGetClipboardString((GLFWwindow*)u);
            };
            io.SetClipboardTextFn = set_clipboard_string;
            io.GetClipboardTextFn = get_clipboard_string;
            io.ClipboardUserData = window.get();

            window.cbs.mouse["imgui"] = [](const GLFWwindow*, const int button, const int action, int) {
                auto& io = ImGui::GetIO();
                if (button >= 0 && button < std::size(io.MouseDown)) {
                    io.MouseDown[button] = action == GLFW_PRESS || action == GLFW_REPEAT;
                }
            };;
            window.cbs.scroll["imgui"] = [](const GLFWwindow*, const double xoffset, const double yoffset) {
                auto& io = ImGui::GetIO();
                io.MouseWheelH += static_cast<float>(xoffset);
                io.MouseWheel += static_cast<float>(yoffset);
            };;
            window.cbs.key["imgui"] = [](const GLFWwindow*, const int key, int, const int action, int) {
                auto& io = ImGui::GetIO();
                io.KeysDown[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
            };;
            window.cbs.chr["imgui"] = [](const GLFWwindow*, const unsigned int c) {
                auto& io = ImGui::GetIO();
                if (c > 0 && c < 0x10000) {
                    io.AddInputCharacter(static_cast<unsigned short>(c));
                }
            };;
            window.cbs.cursor_pos["imgui"] = [](GLFWwindow* w, double x, double y) {
                if (glfwGetWindowAttrib(w, GLFW_FOCUSED) == GLFW_TRUE) {
                    auto& io = ImGui::GetIO();
                    if (io.WantSetMousePos) {
                        glfwSetCursorPos(w, io.MousePos.x, io.MousePos.y);
                    }
                    else {
                        io.MousePos = ImVec2(static_cast<float>(x), static_cast<float>(y));
                    }
                }
            };

            ImGui::StyleColorsDark();
            _vao_handle = vgl::gl::create_vertex_array();
            _vbo_handle = vgl::gl::create_buffer();
            _ebo_handle = vgl::gl::create_buffer();
            glEnableVertexArrayAttrib(_vao_handle, 0);
            glEnableVertexArrayAttrib(_vao_handle, 1);
            glEnableVertexArrayAttrib(_vao_handle, 2);
            glVertexArrayAttribFormat(_vao_handle, 0, 2, GL_FLOAT, GL_FALSE, offsetof(ImDrawVert, pos));
            glVertexArrayAttribFormat(_vao_handle, 1, 2, GL_FLOAT, GL_FALSE, offsetof(ImDrawVert, uv));
            glVertexArrayAttribFormat(_vao_handle, 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(ImDrawVert, col));
            glVertexArrayAttribBinding(_vao_handle, 0, 0);
            glVertexArrayAttribBinding(_vao_handle, 1, 0);
            glVertexArrayAttribBinding(_vao_handle, 2, 0);
        }
        Gui(const Gui&) = delete;
        Gui& operator=(const Gui&) = delete;
        Gui(Gui&&) = default;
        Gui& operator=(Gui&&) = default;
        ~Gui() {
            for (auto& cursor : _mouse_cursors) {
                glfwDestroyCursor(cursor);
            }
            if (_font_texture.id() != 0) {
                auto& io = ImGui::GetIO();
                io.Fonts = nullptr;
            }
            ImGui::DestroyContext();
        };

        void start_frame() {
            auto& io = ImGui::GetIO();
            IM_ASSERT(io.Fonts->IsBuilt());
            // Font atlas needs to be built, call renderer _NewFrame() function e.g. ImGui_ImplOpenGL3_NewFrame() 

             // Setup display size
            glm::ivec2 size;
            glfwGetWindowSize(_window_ptr, &size.x, &size.y);
            const auto size_x = static_cast<float>(size.x);
            const auto size_y = static_cast<float>(size.y);
            glm::ivec2 framebuffer_size;
            glfwGetFramebufferSize(_window_ptr, &framebuffer_size.x, &framebuffer_size.y);
            io.DisplaySize = ImVec2(size_x, size_y);
            io.DisplayFramebufferScale = ImVec2(size_x > 0 ? static_cast<float>(framebuffer_size.x) / size_x : 0,
                size_y > 0 ? static_cast<float>(framebuffer_size.y) / size_y : 0);
            // Setup time step
            const auto current_time = glfwGetTime();
            io.DeltaTime = _time > 0.0 ? static_cast<float>(current_time - _time) : static_cast<float>(1.0f / 60.0f);
            _time = current_time;

            if (!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) && glfwGetInputMode(_window_ptr, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) {
                const auto imgui_cursor = ImGui::GetMouseCursor();
                if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
                    // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
                    glfwSetInputMode(_window_ptr, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                }
                else {
                    // Show OS mouse cursor
                    // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
                    glfwSetCursor(_window_ptr, _mouse_cursors[imgui_cursor] ? _mouse_cursors[imgui_cursor] : _mouse_cursors[ImGuiMouseCursor_Arrow]);
                    glfwSetInputMode(_window_ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
            }

            // Update mods
            io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
            io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
            io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
            io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
            ImGui::NewFrame();
        }

        void render() {
            ImGui::Render();
            glfwMakeContextCurrent(_window_ptr);

            auto& io = ImGui::GetIO();
            auto draw_data = ImGui::GetDrawData();
            const auto fb_width = static_cast<int>(draw_data->DisplaySize.x * io.DisplayFramebufferScale.x);
            const auto fb_height = static_cast<int>(draw_data->DisplaySize.y * io.DisplayFramebufferScale.y);
            if (fb_width <= 0 || fb_height <= 0) {
                return;
            }
            int vertex_buffer_size = 0;
            unsigned int index_buffer_size = 0;
            for (int n = 0; n < draw_data->CmdListsCount; n++) {
                const ImDrawList* cmd_list = draw_data->CmdLists[n];
                index_buffer_size += cmd_list->IdxBuffer.size_in_bytes();
                vertex_buffer_size += cmd_list->VtxBuffer.size_in_bytes();
            }
            if (index_buffer_size == 0 || vertex_buffer_size == 0) {
                return;
            }

            // Backup GL state
            GLint ssbo_last_binding;
            glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 0, &ssbo_last_binding);
            GLint draw_indirect_binding;
            glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &draw_indirect_binding);
            GLint last_polygon_mode[2];
            glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
            GLenum last_blend_src_rgb;
            glGetIntegerv(GL_BLEND_SRC_RGB, reinterpret_cast<GLint*>(&last_blend_src_rgb));
            GLenum last_blend_dst_rgb;
            glGetIntegerv(GL_BLEND_DST_RGB, reinterpret_cast<GLint*>(&last_blend_dst_rgb));
            GLenum last_blend_src_alpha;
            glGetIntegerv(GL_BLEND_SRC_ALPHA, reinterpret_cast<GLint*>(&last_blend_src_alpha));
            GLenum last_blend_dst_alpha;
            glGetIntegerv(GL_BLEND_DST_ALPHA, reinterpret_cast<GLint*>(&last_blend_dst_alpha));
            GLenum last_blend_equation_rgb;
            glGetIntegerv(GL_BLEND_EQUATION_RGB, reinterpret_cast<GLint*>(&last_blend_equation_rgb));
            GLenum last_blend_equation_alpha;
            glGetIntegerv(GL_BLEND_EQUATION_ALPHA, reinterpret_cast<GLint*>(&last_blend_equation_alpha));
            const auto last_enable_blend = glIsEnabled(GL_BLEND);
            const auto last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
            const auto last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
            const auto last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
            std::array<GLint, 4> previous_viewport;
            glGetIntegerv(GL_VIEWPORT, previous_viewport.data());

            glViewport(0, 0, fb_width, fb_height);
            draw_data->ScaleClipRects(io.DisplayFramebufferScale);

            // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_CULL_FACE);
            glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            const auto pos = draw_data->DisplayPos;
            _ebo_handle = vgl::gl::create_buffer();
            gl::set_empty_buffer_storage(_ebo_handle, index_buffer_size, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
            _vbo_handle = vgl::gl::create_buffer();
            gl::set_empty_buffer_storage(_vbo_handle, vertex_buffer_size, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
            std::vector<gl::Indirect_elements_command> draw_cmds;
            std::vector<GLuint64> textures;
            vertex_buffer_size = 0;
            index_buffer_size = 0;
            unsigned int current_base_vertex = 0;
            unsigned int current_draw_count = 0;
            for (int n = 0; n < draw_data->CmdListsCount; n++) {
                const ImDrawList* cmd_list = draw_data->CmdLists[n];
                const auto v_buffer_ptr = glMapNamedBufferRange(_vbo_handle, vertex_buffer_size, cmd_list->VtxBuffer.size_in_bytes(),
                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
                std::memcpy(v_buffer_ptr, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.size_in_bytes());
                glUnmapNamedBuffer(_vbo_handle);
                vertex_buffer_size += cmd_list->VtxBuffer.size_in_bytes();
                const auto i_buffer_ptr = glMapNamedBufferRange(_ebo_handle, index_buffer_size, cmd_list->IdxBuffer.size_in_bytes(),
                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
                std::memcpy(i_buffer_ptr, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.size_in_bytes());
                glUnmapNamedBuffer(_ebo_handle);
                index_buffer_size += cmd_list->IdxBuffer.size_in_bytes();
                for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
                    auto pcmd = &cmd_list->CmdBuffer[cmd_i];
                    if (pcmd->UserCallback) {
                        // User callback (registered via ImDrawList::AddCallback)
                        pcmd->UserCallback(cmd_list, pcmd);
                    }
                    else {
                        const auto clip_rect = ImVec4(pcmd->ClipRect.x - pos.x, pcmd->ClipRect.y - pos.y,
                            pcmd->ClipRect.z - pos.x, pcmd->ClipRect.w - pos.y);
                        if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
                            textures.push_back(gl::get_texture_handle(static_cast<GLuint>(reinterpret_cast<intptr_t>(pcmd->TextureId))));
                            draw_cmds.push_back(gl::Indirect_elements_command{ pcmd->ElemCount, 1, current_draw_count, current_base_vertex, 0 });
                        }
                    }
                    current_draw_count += pcmd->ElemCount;
                }
                current_base_vertex += cmd_list->VtxBuffer.Size;
            }
            auto draw_indirect_buffer = vgl::gl::create_buffer(draw_cmds);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_indirect_buffer);
            auto tex_buffer = gl::create_buffer(textures);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, tex_buffer);
            glVertexArrayElementBuffer(_vao_handle, _ebo_handle);
            glVertexArrayVertexBuffer(_vao_handle, 0, _vbo_handle, 0, sizeof(ImDrawVert));

            const auto l = draw_data->DisplayPos.x;
            const auto r = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
            const auto t = draw_data->DisplayPos.y;
            const auto b = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
            const glm::mat4 ortho_projection =
            {
                {2.0f / (r - l), 0.0f, 0.0f, 0.0f},
                {0.0f, 2.0f / (t - b), 0.0f, 0.0f},
                {0.0f, 0.0f, -1.0f, 0.0f},
                {(r + l) / (l - r), (t + b) / (b - t), 0.0f, 1.0f},
            };
            glUseProgram(_program);
            gl::update_uniform(_program, 0, ortho_projection);
            glBindVertexArray(_vao_handle);

            glMultiDrawElementsIndirect(GL_TRIANGLES, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, nullptr,
                static_cast<unsigned int>(draw_cmds.size()), 0);

            // Restore modified GL state
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_last_binding);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_indirect_binding);
            glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
            glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
            if (last_enable_blend) glEnable(GL_BLEND);
            else glDisable(GL_BLEND);
            if (last_enable_cull_face) glEnable(GL_CULL_FACE);
            else glDisable(GL_CULL_FACE);
            if (last_enable_depth_test) glEnable(GL_DEPTH_TEST);
            else glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(last_polygon_mode[0]));
            glViewport(previous_viewport[0], previous_viewport[1], previous_viewport[2], previous_viewport[3]);
        }

        void setup_fonts(float font_size) {
            auto& io = ImGui::GetIO();
            io.Fonts->Clear();
            ImFontConfig config;
            config.OversampleH = 6;
            config.OversampleV = 3;
            io.Fonts->AddFontFromFileTTF((file::resources_path / "fonts/OpenSans-Regular.ttf").string().c_str(), font_size, &config);
            const std::array<ImWchar, 3> icons_range{ ICON_MIN_MDI, ICON_MAX_MDI, 0 };
            ImFontConfig icons_config;
            icons_config.MergeMode = true;
            icons_config.PixelSnapH = true;
            io.Fonts->AddFontFromFileTTF((file::resources_path / "fonts" / FONT_ICON_FILE_NAME_MDI).string().c_str(),
                font_size, &icons_config, icons_range.data());
            io.Fonts->Build();
            unsigned char* pixel_ptr;
            int width, height;
            io.Fonts->GetTexDataAsRGBA32(&pixel_ptr, &width, &height);
            // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

            _font_texture = gl::create_texture(GL_TEXTURE_2D);
            glTextureParameteri(_font_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(_font_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(_font_texture, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
            glTextureParameteri(_font_texture, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
            glTextureStorage2D(_font_texture, 1, GL_RGBA8, width, height);
            glTextureSubImage2D(_font_texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixel_ptr);
            stbi_image_free(pixel_ptr);

            // Store our identifier
            io.Fonts->TexID = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(_font_texture));

        }
    private:
        gl::glprogram _program;
        gl::gltexture _font_texture;
        gl::glbuffer _vbo_handle{};
        gl::glbuffer _ebo_handle{};
        gl::glvertexarray _vao_handle{};
        std::array<GLFWcursor*, ImGuiMouseCursor_COUNT> _mouse_cursors{};
        double _time;
        GLFWwindow* _window_ptr;
    };
}

namespace ImGui
{
    inline void StyleColorsVS(ImGuiStyle* dst = nullptr) {
        const auto style = dst ? dst : &GetStyle();
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = colors[ImGuiCol_FrameBgHovered];
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.48f, 0.80f, 1.00f);
        colors[ImGuiCol_TitleBg] = colors[ImGuiCol_TitleBgActive];
        colors[ImGuiCol_TitleBgCollapsed] = colors[ImGuiCol_TitleBg];
        colors[ImGuiCol_MenuBarBg] = colors[ImGuiCol_FrameBg];
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.62f, 0.62f, 0.62f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.94f, 0.92f, 0.94f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab] = colors[ImGuiCol_TitleBgActive];
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.11f, 0.59f, 0.92f, 1.00f);
        colors[ImGuiCol_Button] = colors[ImGuiCol_TitleBgActive]; //ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.11f, 0.59f, 0.92f, 1.00f); //ImVec4(0.11f, 0.59f, 0.92f, 1.00f);
        colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_TitleBgActive]; //ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        colors[ImGuiCol_Header] = colors[ImGuiCol_TitleBg];
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.24f, 0.25f, 1.00f);
        colors[ImGuiCol_HeaderActive] = colors[ImGuiCol_TitleBgActive];
        colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.32f, 0.69f, 0.94f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = colors[ImGuiCol_ButtonHovered];//ImVec4(0.53f, 0.19f, 0.78f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.73f, 0.09f, 0.98f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }
    ImTextureID ConvertGLuintToImTex(const GLuint t_id) {
        return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(t_id));
    }
}
