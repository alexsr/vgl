#include "window.hpp"
#include "glad/glad.h"
#include <stdexcept>

vgl::Window::Window(int width, int height, const std::string& title, GLFWmonitor* monitor, GLFWwindow* share) {
    if (_count == 0) {
        glfwInit();
    }
    ++_count;
    _ptr = std::unique_ptr<GLFWwindow, window_deleter>(
        glfwCreateWindow(width, height, title.c_str(), monitor, share));
    init_callbacks(_ptr.get());
    cbs.key["default"] = [this](GLFWwindow*, int k, int s, int a, int m) {
        this->key[k] = a == GLFW_PRESS || a == GLFW_REPEAT;
    };
    cbs.mouse["default"] = [this](GLFWwindow*, int b, int a, int m) {
        this->mouse[b] = a == GLFW_PRESS || a == GLFW_REPEAT;
    };
    cbs.cursor_pos["default"] = [this](GLFWwindow*, double x, double y) {
        glm::dvec2 new_pos{ x, y };
        cursor_delta = new_pos - this->cursor_pos;
        this->cursor_pos = new_pos;
    };
}

void vgl::Window::enable_gl(int major, int minor) const {
    glfwMakeContextCurrent(_ptr.get());
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error{ "Failed to initialize OpenGL." };
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
}

void vgl::Window::poll_events() {
    cursor_delta = glm::dvec2(0.0);
    glfwPollEvents();
}

void vgl::Window::swap_buffers() const {
    glfwSwapBuffers(_ptr.get());
}

bool vgl::Window::should_close() const {
    return glfwWindowShouldClose(_ptr.get());
}

GLFWwindow* vgl::Window::get() const {
    return _ptr.get();
}

void vgl::Window::close() {
    return _ptr.reset();
}

glm::ivec2 vgl::Window::size() {
    glm::ivec2 size;
    glfwGetWindowSize(_ptr.get(), &size.x, &size.y);
    return size;
}

glm::ivec2 vgl::Window::framebuffer_size() {
    glm::ivec2 size;
    glfwGetFramebufferSize(_ptr.get(), &size.x, &size.y);
    return size;
}

void vgl::Window::init_callbacks(GLFWwindow* ptr) {
    glfwSetWindowUserPointer(ptr, static_cast<void*>(this));
    glfwSetCharCallback(ptr, [](GLFWwindow * w, const unsigned c) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.chr) {
            func(w, c);
        }
        });
    glfwSetCharModsCallback(ptr, [](GLFWwindow * w, const unsigned c, const int m) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.chrmods) {
            func(w, c, m);
        }
        });
    glfwSetCursorEnterCallback(ptr, [](GLFWwindow * w, const int e) {
        for (auto& [k, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.cursor_enter) {
            func(w, e);
        }
        });
    glfwSetCursorPosCallback(ptr, [](GLFWwindow * w, const double x, const double y) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.cursor_pos) {
            func(w, x, y);
        }
        });
    glfwSetDropCallback(ptr, [](GLFWwindow * w, const int c, const char** f) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.drop) {
            func(w, c, f);
        }
        });
    glfwSetFramebufferSizeCallback(ptr, [](GLFWwindow * w, const int x, const int y) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.framebuffer_size) {
            func(w, x, y);
        }
        });
    glfwSetKeyCallback(ptr, [](GLFWwindow * w, const int k, const int s, const int a, const int m) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.key) {
            func(w, k, s, a, m);
        }
        });
    glfwSetMouseButtonCallback(ptr, [](GLFWwindow * w, const int k, const int a, const int m) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.mouse) {
            func(w, k, a, m);
        }
        });
    glfwSetScrollCallback(ptr, [](GLFWwindow * w, const double x, const double y) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.scroll) {
            func(w, x, y);
        }
        });
    glfwSetWindowCloseCallback(ptr, [](GLFWwindow * w) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_close) {
            func(w);
        }
        });
    glfwSetWindowContentScaleCallback(ptr, [](GLFWwindow * w, const float x, const float y) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_content_scale) {
            func(w, x, y);
        }
        });
    glfwSetWindowFocusCallback(ptr, [](GLFWwindow * w, const int f) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_focus) {
            func(w, f);
        }
        });
    glfwSetWindowIconifyCallback(ptr, [](GLFWwindow * w, const int i) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_minimize) {
            func(w, i);
        }
        });
    glfwSetWindowMaximizeCallback(ptr, [](GLFWwindow * w, const int m) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_maximize) {
            func(w, m);
        }
        });
    glfwSetWindowPosCallback(ptr, [](GLFWwindow * w, const int x, const int y) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_pos) {
            func(w, x, y);
        }
        });
    glfwSetWindowRefreshCallback(ptr, [](GLFWwindow * w) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_refresh) {
            func(w);
        }
        });
    glfwSetWindowSizeCallback(ptr, [](GLFWwindow * w, const int x, const int y) {
        for (auto& [key, func] : static_cast<Window*>(glfwGetWindowUserPointer(w))->cbs.window_size) {
            func(w, x, y);
        }
        });
}
