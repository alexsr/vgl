#pragma once

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include <map>
#include <string>
#include <functional>

namespace vgl {
    namespace win {
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

        struct window_handler {
            window_handler(GLFWwindow* ptr) : _ptr(ptr) {
                init_callbacks(_ptr);
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
            void poll_events() {
                cursor_delta = glm::dvec2(0.0);
                glfwPollEvents();
            }
            void init_callbacks(GLFWwindow* ptr) {
                glfwSetWindowUserPointer(ptr, static_cast<void*>(this));
                glfwSetCharCallback(ptr, [](GLFWwindow* w, const unsigned c) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.chr) {
                        func(w, c);
                    }
                });
                glfwSetCharModsCallback(ptr, [](GLFWwindow* w, const unsigned c, const int m) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.chrmods) {
                        func(w, c, m);
                    }
                });
                glfwSetCursorEnterCallback(ptr, [](GLFWwindow* w, const int e) {
                    for (auto& [k,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.cursor_enter) {
                        func(w, e);
                    }
                });
                glfwSetCursorPosCallback(ptr, [](GLFWwindow* w, const double x, const double y) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.cursor_pos) {
                        func(w, x, y);
                    }
                });
                glfwSetDropCallback(ptr, [](GLFWwindow* w, const int c, const char** f) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.drop) {
                        func(w, c, f);
                    }
                });
                glfwSetFramebufferSizeCallback(ptr, [](GLFWwindow* w, const int x, const int y) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.framebuffer_size) {
                        func(w, x, y);
                    }
                });
                glfwSetKeyCallback(ptr, [](GLFWwindow* w, const int k, const int s, const int a, const int m) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.key) {
                        func(w, k, s, a, m);
                    }
                });
                glfwSetMouseButtonCallback(ptr, [](GLFWwindow* w, const int k, const int a, const int m) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.mouse) {
                        func(w, k, a, m);
                    }
                });
                glfwSetScrollCallback(ptr, [](GLFWwindow* w, const double x, const double y) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.scroll) {
                        func(w, x, y);
                    }
                });
                glfwSetWindowCloseCallback(ptr, [](GLFWwindow* w) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.window_close) {
                        func(w);
                    }
                });
                glfwSetWindowContentScaleCallback(ptr, [](GLFWwindow* w, const float x, const float y) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.window_content_scale) {
                        func(w, x, y);
                    }
                });
                glfwSetWindowFocusCallback(ptr, [](GLFWwindow* w, const int f) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.window_focus) {
                        func(w, f);
                    }
                });
                glfwSetWindowIconifyCallback(ptr, [](GLFWwindow* w, const int i) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.window_minimize) {
                        func(w, i);
                    }
                });
                glfwSetWindowMaximizeCallback(ptr, [](GLFWwindow* w, const int m) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.window_maximize) {
                        func(w, m);
                    }
                });
                glfwSetWindowPosCallback(ptr, [](GLFWwindow* w, const int x, const int y) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.window_pos) {
                        func(w, x, y);
                    }
                });
                glfwSetWindowRefreshCallback(ptr, [](GLFWwindow* w) {
                    for (auto& [key,func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.window_refresh) {
                        func(w);
                    }
                });
                glfwSetWindowSizeCallback(ptr, [](GLFWwindow * w, const int x, const int y) {
                    for (auto& [key, func] : static_cast<window_handler*>(glfwGetWindowUserPointer(w))->cbs.window_size) {
                        func(w, x, y);
                    }
                });
            }
            GLFWwindow* _ptr;
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
            } cbs;
            std::map<int, bool> key{};
            std::map<int, bool> mouse{};
            glm::dvec2 cursor_pos;
            glm::dvec2 cursor_delta;
        };

        /*void add_mouse_callback(const std::string& name, const mouse_callback& fun) {
            const auto res = _callbacks.mouse.insert_or_assign(name, fun);
            init_callbacks();
            if (res.second) {
                log_info() << "Mouse callback \"" << name << "\" registered.";
            }
            else {
                log_warning() << "Mouse callback \"" << name << "\" was overwritten.";
            }
        }

        void add_scroll_callback(const std::string& name, const scroll_callback& fun) {
            const auto res = _callbacks.scroll.insert_or_assign(name, fun);
            init_callbacks();
            if (res.second) {
                log_info() << "Scroll callback \"" << name << "\" registered.";
            }
            else {
                log_warning() << "Scroll callback \"" << name << "\" was overwritten.";
            }
        }

        void tostf::win_mgr::win_obj::add_key_callback(const std::string& name, const key_callback& fun) {
            const auto res = _callbacks.key.insert_or_assign(name, fun);
            init_callbacks();
            if (res.second) {
                log_info() << "Key callback \"" << name << "\" registered.";
            }
            else {
                log_warning() << "Key callback \"" << name << "\" was overwritten.";
            }
        }

        void tostf::win_mgr::win_obj::add_char_callback(const std::string& name, const chr_callback& fun) {
            const auto res = _callbacks.chr.insert_or_assign(name, fun);
            init_callbacks();
            if (res.second) {
                log_info() << "Char callback \"" << name << "\" registered.";
            }
            else {
                log_warning() << "Char callback \"" << name << "\" was overwritten.";
            }
        }
*/
        /*void tostf::win_mgr::win_obj::add_resize_callback(const std::string& name, const window_size_callback& fun) {
            const auto res = _callbacks.window_size.insert_or_assign(name, fun);
            init_callbacks();
            if (res.second) {
                log_info() << "Resize callback \"" << name << "\" registered.";
            }
            else {
                log_warning() << "Resize callback \"" << name << "\" was overwritten.";
            }
        }*/
/*
        void tostf::win_mgr::win_obj::remove_mouse_callback(const std::string& name) {
            const auto it = _callbacks.mouse.find(name);
            if (it != _callbacks.mouse.end()) {
                _callbacks.mouse.erase(it);
                log_info() << "Mouse callback \"" << name << "\" was removed.";
            }
            else {
                log_warning() << "Mouse callback \"" << name << "\" could not be found.";
            }
        }

        void tostf::win_mgr::win_obj::remove_scroll_callback(const std::string& name) {
            const auto it = _callbacks.scroll.find(name);
            if (it != _callbacks.scroll.end()) {
                _callbacks.scroll.erase(it);
                log_info() << "Scroll callback \"" << name << "\" was removed.";
            }
            else {
                log_warning() << "Scroll callback \"" << name << "\" could not be found.";
            }
        }

        void tostf::win_mgr::win_obj::remove_key_callback(const std::string& name) {
            const auto it = _callbacks.key.find(name);
            if (it != _callbacks.key.end()) {
                _callbacks.key.erase(it);
                log_info() << "Key callback \"" << name << "\" was removed.";
            }
            else {
                log_warning() << "Key callback \"" << name << "\" could not be found.";
            }
        }

        void tostf::win_mgr::win_obj::remove_char_callback(const std::string& name) {
            const auto it = _callbacks.chr.find(name);
            if (it != _callbacks.chr.end()) {
                _callbacks.chr.erase(it);
                log_info() << "Char callback \"" << name << "\" was removed.";
            }
            else {
                log_warning() << "Char callback \"" << name << "\" could not be found.";
            }
        }

        void tostf::win_mgr::win_obj::remove_resize_callback(const std::string& name) {
            const auto it = _callbacks.window_size.find(name);
            if (it != _callbacks.window_size.end()) {
                _callbacks.window_size.erase(it);
                log_info() << "Resize callback \"" << name << "\" was removed.";
            }
            else {
                log_warning() << "Resize callback \"" << name << "\" could not be found.";
            }
        }
*/
    }
}