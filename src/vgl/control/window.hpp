#pragma once

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include <map>
#include <string>
#include <functional>
#include "glad/glad.h"
#include <stdexcept>
#include <memory>
#include <atomic>

namespace vgl
{
    namespace win
    {
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
    }

    class window {
    public:
        window(int width, int height, const std::string& title, GLFWmonitor* monitor = nullptr,
               GLFWwindow* share = nullptr) {
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
                glm::dvec2 new_pos{x, y};
                cursor_delta = new_pos - this->cursor_pos;
                this->cursor_pos = new_pos;
            };
        }

        void enable_gl(int major = 4, int minor = 6) const {
            glfwMakeContextCurrent(_ptr.get());
            if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
                throw std::runtime_error{"Failed to initialize OpenGL."};
            }
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
        }

        void poll_events() {
            cursor_delta = glm::dvec2(0.0);
            glfwPollEvents();
        }

        void swap_buffers() const {
            glfwSwapBuffers(_ptr.get());
        }

        bool should_close() const {
            return glfwWindowShouldClose(_ptr.get());
        }

        GLFWwindow* get() const {
            return _ptr.get();
        }
        void close() {
            return _ptr.reset();
        }

        glm::ivec2 size() {
            glm::ivec2 size;
            glfwGetWindowSize(_ptr.get(), &size.x, &size.y);
            return size;
        }

        glm::ivec2 framebuffer_size() {
            glm::ivec2 size;
            glfwGetFramebufferSize(_ptr.get(), &size.x, &size.y);
            return size;
        }

        struct callbacks {
            std::map<std::string, win::chr_callback> chr;
            std::map<std::string, win::chrmods_callback> chrmods;
            std::map<std::string, win::cursor_enter_callback> cursor_enter;
            std::map<std::string, win::cursor_pos_callback> cursor_pos;
            std::map<std::string, win::drop_callback> drop;
            std::map<std::string, win::framebuffer_size_callback> framebuffer_size;
            std::map<std::string, win::key_callback> key;
            std::map<std::string, win::mouse_callback> mouse;
            std::map<std::string, win::scroll_callback> scroll;
            std::map<std::string, win::window_close_callback> window_close;
            std::map<std::string, win::window_content_scale_callback> window_content_scale;
            std::map<std::string, win::window_focus_callback> window_focus;
            std::map<std::string, win::window_minimize_callback> window_minimize;
            std::map<std::string, win::window_maximize_callback> window_maximize;
            std::map<std::string, win::window_pos_callback> window_pos;
            std::map<std::string, win::window_refresh_callback> window_refresh;
            std::map<std::string, win::window_size_callback> window_size;
        } cbs;

        std::map<int, bool> key{};
        std::map<int, bool> mouse{};
        glm::dvec2 cursor_pos{};
        glm::dvec2 cursor_delta{};
    private:
        void init_callbacks(GLFWwindow* ptr) {
            glfwSetWindowUserPointer(ptr, static_cast<void*>(this));
            glfwSetCharCallback(ptr, [](GLFWwindow* w, const unsigned c) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.chr) {
                    func(w, c);
                }
            });
            glfwSetCharModsCallback(ptr, [](GLFWwindow* w, const unsigned c, const int m) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.chrmods) {
                    func(w, c, m);
                }
            });
            glfwSetCursorEnterCallback(ptr, [](GLFWwindow* w, const int e) {
                for (auto& [k,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.cursor_enter) {
                    func(w, e);
                }
            });
            glfwSetCursorPosCallback(ptr, [](GLFWwindow* w, const double x, const double y) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.cursor_pos) {
                    func(w, x, y);
                }
            });
            glfwSetDropCallback(ptr, [](GLFWwindow* w, const int c, const char** f) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.drop) {
                    func(w, c, f);
                }
            });
            glfwSetFramebufferSizeCallback(ptr, [](GLFWwindow* w, const int x, const int y) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.framebuffer_size) {
                    func(w, x, y);
                }
            });
            glfwSetKeyCallback(ptr, [](GLFWwindow* w, const int k, const int s, const int a, const int m) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.key) {
                    func(w, k, s, a, m);
                }
            });
            glfwSetMouseButtonCallback(ptr, [](GLFWwindow* w, const int k, const int a, const int m) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.mouse) {
                    func(w, k, a, m);
                }
            });
            glfwSetScrollCallback(ptr, [](GLFWwindow* w, const double x, const double y) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.scroll) {
                    func(w, x, y);
                }
            });
            glfwSetWindowCloseCallback(ptr, [](GLFWwindow* w) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.window_close) {
                    func(w);
                }
            });
            glfwSetWindowContentScaleCallback(ptr, [](GLFWwindow* w, const float x, const float y) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.window_content_scale) {
                    func(w, x, y);
                }
            });
            glfwSetWindowFocusCallback(ptr, [](GLFWwindow* w, const int f) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.window_focus) {
                    func(w, f);
                }
            });
            glfwSetWindowIconifyCallback(ptr, [](GLFWwindow* w, const int i) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.window_minimize) {
                    func(w, i);
                }
            });
            glfwSetWindowMaximizeCallback(ptr, [](GLFWwindow* w, const int m) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.window_maximize) {
                    func(w, m);
                }
            });
            glfwSetWindowPosCallback(ptr, [](GLFWwindow* w, const int x, const int y) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.window_pos) {
                    func(w, x, y);
                }
            });
            glfwSetWindowRefreshCallback(ptr, [](GLFWwindow* w) {
                for (auto& [key,func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.window_refresh) {
                    func(w);
                }
            });
            glfwSetWindowSizeCallback(ptr, [](GLFWwindow* w, const int x, const int y) {
                for (auto& [key, func] : static_cast<window*>(glfwGetWindowUserPointer(w))->cbs.window_size) {
                    func(w, x, y);
                }
            });
        }

        struct window_deleter {
            void operator()(GLFWwindow* w) const {
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
