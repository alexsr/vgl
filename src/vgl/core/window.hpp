#pragma once

#include <GLFW/glfw3.h>
#include <Eigen/Core>
#include <atomic>
#include <memory>
#include "win_callbacks.hpp"

namespace vgl
{
    class Window {
    public:
        Window(int width, int height, const std::string& title, GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr);
        void enable_gl(int major = 4, int minor = 6);
        void swap_interval(int interval = 0);
        void poll_events();
        void swap_buffers() const;
        bool should_close() const;
        GLFWwindow* get() const;
        void close();
        Eigen::Vector2i size();
        Eigen::Vector2i framebuffer_size();
        operator GLFWwindow*();

        win::callbacks cbs;
        std::map<int, bool> key{};
        std::map<int, bool> mouse{};
        Eigen::Vector2d cursor_pos{};
        Eigen::Vector2d cursor_delta{};
    private:
        void init_callbacks(GLFWwindow* ptr);

        struct window_deleter {
            inline void operator()(GLFWwindow* w) const {
                glfwDestroyWindow(w);
                --count_;
                if (count_ == 0) {
                    glfwTerminate();
                }
            }
        };

        std::unique_ptr<GLFWwindow, window_deleter> ptr_;
        inline static std::atomic<int> count_ = 0;
    };
}
