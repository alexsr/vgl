#include "window.hpp"
#include "glad/glad.h"
#include <stdexcept>

vgl::Window::Window(int width, int height, const std::string& title, GLFWmonitor* monitor, GLFWwindow* share) {
    if (count_ == 0) {
        glfwInit();
    }
    ++count_;
    ptr_ = std::unique_ptr<GLFWwindow, window_deleter>(
        glfwCreateWindow(width, height, title.c_str(), monitor, share));
    init_callbacks(ptr_.get());
    cbs.key["default"] = [this](GLFWwindow*, int k, int s, int a, int m) {
        this->key[k] = a == GLFW_PRESS || a == GLFW_REPEAT;
    };
    cbs.mouse["default"] = [this](GLFWwindow*, int b, int a, int m) {
        this->mouse[b] = a == GLFW_PRESS || a == GLFW_REPEAT;
    };
    cbs.cursor_pos["default"] = [this](GLFWwindow*, double x, double y) {
        Eigen::Vector2d new_pos{ x, y };
        cursor_delta = new_pos - this->cursor_pos;
        this->cursor_pos = new_pos;
    };
}

void vgl::Window::enable_gl(int major, int minor) {
    glfwMakeContextCurrent(ptr_.get());
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error{ "Failed to initialize OpenGL." };
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);

    cbs.framebuffer_size["default"] = [&](GLFWwindow*, int x, int y) {
        if (x > 0 && y > 0) {
            glViewport(0, 0, x, y);
        }
    };
}

void vgl::Window::swap_interval(int interval) {
    glfwSwapInterval(interval);
}

void vgl::Window::poll_events() {
    cursor_delta = Eigen::Vector2d::Zero();
    glfwPollEvents();
}

void vgl::Window::swap_buffers() const {
    glfwSwapBuffers(ptr_.get());
}

bool vgl::Window::should_close() const {
    return glfwWindowShouldClose(ptr_.get());
}

GLFWwindow* vgl::Window::get() const {
    return ptr_.get();
}

void vgl::Window::close() {
    return ptr_.reset();
}

Eigen::Vector2i vgl::Window::size() {
    Eigen::Vector2i win_size;
    glfwGetWindowSize(ptr_.get(), &win_size.x(), &win_size.y());
    return win_size;
}

Eigen::Vector2i vgl::Window::framebuffer_size() {
    Eigen::Vector2i fb_size;
    glfwGetFramebufferSize(ptr_.get(), &fb_size.x(), &fb_size.y());
    return fb_size;
}

vgl::Window::operator GLFWwindow* () {
    return ptr_.get();
}

void vgl::Window::init_callbacks(GLFWwindow* ptr) {
    glfwSetWindowUserPointer(ptr, static_cast<void*>(this));
    glfwSetCharCallback(ptr, [](GLFWwindow* w, const unsigned c) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.chr) {
            func(w, c);
        }
        });
    glfwSetCharModsCallback(ptr, [](GLFWwindow* w, const unsigned c, const int m) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.chrmods) {
            func(w, c, m);
        }
        });
    glfwSetCursorEnterCallback(ptr, [](GLFWwindow* w, const int e) {
        for (auto& [k, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.cursor_enter) {
            func(w, e);
        }
        });
    glfwSetCursorPosCallback(ptr, [](GLFWwindow* w, const double x, const double y) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.cursor_pos) {
            func(w, x, y);
        }
        });
    glfwSetDropCallback(ptr, [](GLFWwindow* w, const int c, const char** f) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.drop) {
            func(w, c, f);
        }
        });
    glfwSetFramebufferSizeCallback(ptr, [](GLFWwindow* w, const int x, const int y) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.framebuffer_size) {
            func(w, x, y);
        }
        });
    glfwSetKeyCallback(ptr, [](GLFWwindow* w, const int k, const int s, const int a, const int m) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.key) {
            func(w, k, s, a, m);
        }
        });
    glfwSetMouseButtonCallback(ptr, [](GLFWwindow* w, const int k, const int a, const int m) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.mouse) {
            func(w, k, a, m);
        }
        });
    glfwSetScrollCallback(ptr, [](GLFWwindow* w, const double x, const double y) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.scroll) {
            func(w, x, y);
        }
        });
    glfwSetWindowCloseCallback(ptr, [](GLFWwindow* w) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_close) {
            func(w);
        }
        });
    glfwSetWindowContentScaleCallback(ptr, [](GLFWwindow* w, const float x, const float y) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_content_scale) {
            func(w, x, y);
        }
        });
    glfwSetWindowFocusCallback(ptr, [](GLFWwindow* w, const int f) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_focus) {
            func(w, f);
        }
        });
    glfwSetWindowIconifyCallback(ptr, [](GLFWwindow* w, const int i) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_minimize) {
            func(w, i);
        }
        });
    glfwSetWindowMaximizeCallback(ptr, [](GLFWwindow* w, const int m) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_maximize) {
            func(w, m);
        }
        });
    glfwSetWindowPosCallback(ptr, [](GLFWwindow* w, const int x, const int y) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_pos) {
            func(w, x, y);
        }
        });
    glfwSetWindowRefreshCallback(ptr, [](GLFWwindow* w) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_refresh) {
            func(w);
        }
        });
    glfwSetWindowSizeCallback(ptr, [](GLFWwindow* w, const int x, const int y) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_size) {
            func(w, x, y);
        }
        });
}
