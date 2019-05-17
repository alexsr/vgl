#pragma once

#include "GLFW/glfw3.h"
#include "glm/vec2.hpp"
#include <atomic>
#include <memory>
#include "win_callbacks.hpp"

namespace vgl
{
    class Window {
    public:
        Window(int width, int height, const std::string& title, GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr);
        void enable_gl(int major = 4, int minor = 6) const;
        void poll_events();
        void swap_buffers() const;
        bool should_close() const;
        GLFWwindow* get() const;
        void close();
        glm::ivec2 size();
        glm::ivec2 framebuffer_size();

        win::callbacks cbs;
        std::map<int, bool> key{};
        std::map<int, bool> mouse{};
        glm::dvec2 cursor_pos{};
        glm::dvec2 cursor_delta{};
    private:
        void init_callbacks(GLFWwindow* ptr);

        struct window_deleter {
            inline void operator()(GLFWwindow* w) const {
                glfwDestroyWindow(w);
                --_count;
                if (_count == 0) {
                    glfwTerminate();
                }
            }
        };

        std::unique_ptr<GLFWwindow, window_deleter> _ptr;
        inline static std::atomic<int> _count = 0;
    };
}
