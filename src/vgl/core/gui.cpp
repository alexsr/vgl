#include "gui.hpp"
#include "window.hpp"
#include <utility>
#include <string_view>
#include "vgl/file/file_paths.hpp"
#include "vgl/gpu_api/gl/texture.hpp"
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/gpu_api/gl/buffer.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include "vgl/file/image_file.hpp"
#include "vgl/math/projection.hpp"
#include <iconfont/IconsMaterialDesignIcons.h>

namespace gui_detail {
constexpr std::string_view imgui_vs = R"(#version 460
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
constexpr std::string_view imgui_fs = R"(#version 460

#extension GL_ARB_bindless_texture : require

in vec2 Frag_UV;
in vec4 Frag_Color;
in flat int draw_id;
out vec4 Out_Color;

layout (location = 1) uniform sampler2D tex;

void main() {
    Out_Color = Frag_Color * texture(tex, Frag_UV.st);
})";
} // namespace gui_detail

vgl::ui::Gui::Gui(vgl::Window& window, float font_size, bool install_callbacks_flag) {
    window_ptr_ = window.get();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0;
    style.ScrollbarRounding = 0.0f;
    style.ScrollbarSize = 20.0f;
    style.DisplaySafeAreaPadding = ImVec2(0.0f, 0.0f);
    style.DisplayWindowPadding = ImVec2(0.0f, 0.0f);
    style.ChildBorderSize = 1.0f;
    ImGui::StyleColorsVS();

    auto& io = ImGui::GetIO();
    auto vs = gl::create_shader(GL_VERTEX_SHADER, gui_detail::imgui_vs.data());
    auto fs = gl::create_shader(GL_FRAGMENT_SHADER, gui_detail::imgui_fs.data());
    program_ = gl::create_program({vs, fs});
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
    mouse_cursors_[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouse_cursors_[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    mouse_cursors_[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouse_cursors_[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    mouse_cursors_[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    mouse_cursors_[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouse_cursors_[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouse_cursors_[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

    auto set_clipboard_string = [](void* u, const char* c) {
        glfwSetClipboardString(reinterpret_cast<GLFWwindow*>(u), c);
    };
    auto get_clipboard_string = [](void* u) -> auto {
        return glfwGetClipboardString(reinterpret_cast<GLFWwindow*>(u));
    };
    io.SetClipboardTextFn = set_clipboard_string;
    io.GetClipboardTextFn = get_clipboard_string;
    io.ClipboardUserData = window.get();

    window.cbs.mouse["imgui"] = [](const GLFWwindow*, const int button, const int action, int) {
        auto& io = ImGui::GetIO();
        if (button >= 0 && button < std::size(io.MouseDown)) {
            io.MouseDown[button] = action == GLFW_PRESS || action == GLFW_REPEAT;
        }
    };
    window.cbs.scroll["imgui"] = [](const GLFWwindow*, const double xoffset, const double yoffset) {
        auto& io = ImGui::GetIO();
        io.MouseWheelH += static_cast<float>(xoffset);
        io.MouseWheel += static_cast<float>(yoffset);
    };
    window.cbs.key["imgui"] = [](const GLFWwindow*, const int key, int, const int action, int) {
        auto& io = ImGui::GetIO();
        io.KeysDown[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
    };
    window.cbs.chr["imgui"] = [](const GLFWwindow*, const unsigned int c) {
        auto& io = ImGui::GetIO();
        if (c > 0 && c < 0x10000) {
            io.AddInputCharacter(static_cast<unsigned short>(c));
        }
    };
    window.cbs.cursor_pos["imgui"] = [](GLFWwindow* w, double x, double y) {
        if (glfwGetWindowAttrib(w, GLFW_FOCUSED) == GLFW_TRUE) {
            auto& io = ImGui::GetIO();
            if (io.WantSetMousePos) {
                glfwSetCursorPos(w, io.MousePos.x, io.MousePos.y);
            } else {
                io.MousePos = ImVec2(static_cast<float>(x), static_cast<float>(y));
            }
        }
    };

    vao_handle_ = vgl::gl::create_vertex_array();
    vbo_handle_ = vgl::gl::create_buffer();
    ebo_handle_ = vgl::gl::create_buffer();
    glGenBuffers(1, &vbo_handle_);
    glGenBuffers(1, &ebo_handle_);
    glGenVertexArrays(1, &vao_handle_);
    glBindVertexArray(vao_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_handle_);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                          reinterpret_cast<GLvoid*>(IM_OFFSETOF(ImDrawVert, pos)));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                          reinterpret_cast<GLvoid*>(IM_OFFSETOF(ImDrawVert, uv)));
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
                          reinterpret_cast<GLvoid*>(IM_OFFSETOF(ImDrawVert, col)));
}
vgl::ui::Gui::~Gui() {
    for (auto& cursor : mouse_cursors_) {
        glfwDestroyCursor(cursor);
    }
    if (font_texture_.id() != 0) {
        auto& io = ImGui::GetIO();
        io.Fonts = nullptr;
    }
    ImGui::DestroyContext();
};

void vgl::ui::Gui::start_frame() {
    auto& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt());
    // Font atlas needs to be built, call renderer _NewFrame() function e.g. ImGui_ImplOpenGL3_NewFrame()

    // Setup display size
    int size_x, size_y;
    glfwGetWindowSize(window_ptr_, &size_x, &size_y);
    Eigen::Vector2f size(size_x, size_y);

    Eigen::Vector2i framebuffer_size;
    glfwGetFramebufferSize(window_ptr_, &framebuffer_size.x(), &framebuffer_size.y());
    io.DisplaySize = ImVec2(size.x(), size.y());
    io.DisplayFramebufferScale = ImVec2(size.x() > 0 ? static_cast<float>(framebuffer_size.x()) / size.x() : 0,
                                        size.y() > 0 ? static_cast<float>(framebuffer_size.y()) / size.y() : 0);
    // Setup time step
    const auto current_time = glfwGetTime();
    io.DeltaTime = time_ > 0.0 ? static_cast<float>(current_time - time_) : static_cast<float>(1.0f / 60.0f);
    time_ = current_time;

    if (!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) &&
        glfwGetInputMode(window_ptr_, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) {
        const auto imgui_cursor = ImGui::GetMouseCursor();
        if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
            // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
            glfwSetInputMode(window_ptr_, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        } else {
            // Show OS mouse cursor
            // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works
            // here.
            glfwSetCursor(window_ptr_, mouse_cursors_[imgui_cursor] ? mouse_cursors_[imgui_cursor]
                                                                    : mouse_cursors_[ImGuiMouseCursor_Arrow]);
            glfwSetInputMode(window_ptr_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    // Update mods
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
    ImGui::NewFrame();
}

void vgl::ui::Gui::render() {
    ImGui::Render();
    glfwMakeContextCurrent(window_ptr_);

    auto& io = ImGui::GetIO();
    auto draw_data = ImGui::GetDrawData();
    const auto fb_width = static_cast<int>(draw_data->DisplaySize.x * io.DisplayFramebufferScale.x);
    const auto fb_height = static_cast<int>(draw_data->DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) {
        return;
    }

    // Backup GL state
    GLenum last_active_texture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, reinterpret_cast<GLint*>(&last_active_texture));
    glActiveTexture(GL_TEXTURE0);
    GLint last_polygon_mode[2];
    glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
    GLint last_scissor_box[4];
    glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
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
    glEnable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    const auto l = draw_data->DisplayPos.x;
    const auto r = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    const auto t = draw_data->DisplayPos.y;
    const auto b = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    Eigen::Matrix4f ortho_projection = vgl::orthographic_projection<float>(l, r, t, b, -1.0, 1.0);
    glUseProgram(program_);
    gl::update_uniform(program_, 0, ortho_projection);
    gl::update_uniform(program_, 1, 0);
    glBindVertexArray(vao_handle_);
    // Draw
    const auto pos = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = nullptr;

        glBindBuffer(GL_ARRAY_BUFFER, vbo_handle_);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cmd_list->VtxBuffer.Size) * sizeof(ImDrawVert),
                     static_cast<const GLvoid*>(cmd_list->VtxBuffer.Data), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_handle_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cmd_list->IdxBuffer.Size) * sizeof(ImDrawIdx),
                     static_cast<const GLvoid*>(cmd_list->IdxBuffer.Data), GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            auto pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                // User callback (registered via ImDrawList::AddCallback)
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                const auto clip_rect = ImVec4(pcmd->ClipRect.x - pos.x, pcmd->ClipRect.y - pos.y,
                                              pcmd->ClipRect.z - pos.x, pcmd->ClipRect.w - pos.y);
                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
                    // Apply scissor/clipping rectangle
                    glScissor(static_cast<int>(clip_rect.x), static_cast<int>(fb_height - clip_rect.w),
                              static_cast<int>(clip_rect.z - clip_rect.x), static_cast<int>(clip_rect.w - clip_rect.y));

                    // Bind texture, Draw
                    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(pcmd->TextureId)));
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(pcmd->ElemCount),
                                   sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
                }
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    // Restore modified GL state
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    if (last_enable_blend)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
    if (last_enable_cull_face)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
    if (last_enable_depth_test)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
    glScissor(last_scissor_box[0], last_scissor_box[1], static_cast<GLsizei>(last_scissor_box[2]),
              static_cast<GLsizei>(last_scissor_box[3]));
    glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(last_polygon_mode[0]));
    glViewport(previous_viewport[0], previous_viewport[1], previous_viewport[2], previous_viewport[3]);
}

void vgl::ui::Gui::setup_fonts(float font_size) {
    auto& io = ImGui::GetIO();
    io.Fonts->Clear();
    ImFontConfig config;
    config.OversampleH = 6;
    config.OversampleV = 3;
    io.Fonts->AddFontFromFileTTF((file::resources_path / "fonts/OpenSans-Regular.ttf").string().c_str(), font_size,
                                 &config);
    const std::array<ImWchar, 3> icons_range{ICON_MIN_MDI, ICON_MAX_MDI, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF((file::resources_path / "fonts/" / FONT_ICON_FILE_NAME_MDI).string().c_str(),
                                 font_size, &icons_config, icons_range.data());
    io.Fonts->Build();
    unsigned char* pixel_ptr;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixel_ptr, &width, &height);
    // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be
    // compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL
    // texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    font_texture_ = gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(font_texture_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(font_texture_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(font_texture_, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(font_texture_, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(font_texture_, 1, GL_RGBA8, width, height);
    glTextureSubImage2D(font_texture_, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixel_ptr);
    stbi_image_free(pixel_ptr);

    // Store our identifier
    io.Fonts->TexID = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(font_texture_));
}

void ImGui::StyleColorsVS(ImGuiStyle* dst) {
    const auto style = dst ? dst : &GetStyle();
    style->WindowRounding = 0;
    style->ScrollbarRounding = 0.0f;
    style->ScrollbarSize = 20.0f;
    style->DisplaySafeAreaPadding = ImVec2(0.0f, 0.0f);
    style->DisplayWindowPadding = ImVec2(0.0f, 0.0f);
    style->ChildBorderSize = 1.0f;
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
    colors[ImGuiCol_Button] = colors[ImGuiCol_TitleBgActive];            // ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.11f, 0.59f, 0.92f, 1.00f); // ImVec4(0.11f, 0.59f, 0.92f, 1.00f);
    colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_TitleBgActive];      // ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
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
    colors[ImGuiCol_PlotHistogram] = colors[ImGuiCol_ButtonHovered]; // ImVec4(0.53f, 0.19f, 0.78f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.73f, 0.09f, 0.98f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
