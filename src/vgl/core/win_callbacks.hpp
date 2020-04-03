#pragma once

#include <string>
#include <map>
#include <functional>

struct GLFWwindow;

namespace vgl::win {
    using chr_callback = std::function<void(GLFWwindow*, unsigned int)>;
    using chrmods_callback = std::function<void(GLFWwindow*, unsigned int, int)>;
    using cursor_enter_callback = std::function<void(GLFWwindow*, int)>;
    using cursor_pos_callback = std::function<void(GLFWwindow*, double, double)>;
    using drop_callback = std::function<void(GLFWwindow*, int, const char**)>;
    using framebuffer_size_callback = std::function<void(GLFWwindow*, int, int)>;
    using key_callback = std::function<void(GLFWwindow*, int, int, int, int)>;
    using mouse_callback = std::function<void(GLFWwindow*, int, int, int)>;
    using scroll_callback = std::function<void(GLFWwindow*, double, double)>;
    using window_close_callback = std::function<void(GLFWwindow*)>;
    using window_content_scale_callback = std::function<void(GLFWwindow*, float, float)>;
    using window_focus_callback = std::function<void(GLFWwindow*, int)>;
    using window_minimize_callback = std::function<void(GLFWwindow*, int)>;
    using window_maximize_callback = std::function<void(GLFWwindow*, int)>;
    using window_pos_callback = std::function<void(GLFWwindow*, int, int)>;
    using window_refresh_callback = std::function<void(GLFWwindow*)>;
    using window_size_callback = std::function<void(GLFWwindow*, int, int)>;

    struct callbacks {
        std::map<std::string, chr_callback> chr;
        std::map<std::string, chrmods_callback> chrmods;
        std::map<std::string, cursor_enter_callback> cursor_enter;
        std::map<std::string, cursor_pos_callback> cursor_pos;
        std::map<std::string, drop_callback> drop;
        std::map<std::string, framebuffer_size_callback> framebuffer_size;
        std::map<std::string, key_callback> key;
        std::map<std::string, mouse_callback> mouse;
        std::map<std::string, scroll_callback> scroll;
        std::map<std::string, window_close_callback> window_close;
        std::map<std::string, window_content_scale_callback> window_content_scale;
        std::map<std::string, window_focus_callback> window_focus;
        std::map<std::string, window_minimize_callback> window_minimize;
        std::map<std::string, window_maximize_callback> window_maximize;
        std::map<std::string, window_pos_callback> window_pos;
        std::map<std::string, window_refresh_callback> window_refresh;
        std::map<std::string, window_size_callback> window_size;
    };
}