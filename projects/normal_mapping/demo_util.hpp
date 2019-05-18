#pragma once

#include "vgl/control/window.hpp"
#include "vgl/rendering/camera.hpp"
#include "imgui/imgui.h"

namespace demo {
    inline void hide_cursor(vgl::Window& window) {
        if (!ImGui::IsAnyItemHovered() && !ImGui::IsAnyWindowHovered() && !ImGui::IsAnyItemFocused()
            && !ImGui::IsAnyItemActive()) {
            if (window.mouse[GLFW_MOUSE_BUTTON_LEFT]) {
                glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else {
                glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    }

    inline void update_cam(vgl::Camera& cam, vgl::Window& window, const float dt) {
        if (!ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive()) {
            glm::vec3 dir(static_cast<float>(window.key[GLFW_KEY_D]) - static_cast<float>(window.key[GLFW_KEY_A]),
                static_cast<float>(window.key[GLFW_KEY_E]) - static_cast<float>(window.key[GLFW_KEY_Q]),
                static_cast<float>(window.key[GLFW_KEY_S]) - static_cast<float>(window.key[GLFW_KEY_W]));
            if (dot(dir, dir) != 0.0f) {
                cam.move(glm::normalize(dir), dt);
            }
            if (!ImGui::IsAnyItemHovered() && !ImGui::IsAnyWindowHovered() && window.mouse[GLFW_MOUSE_BUTTON_LEFT]) {
                cam.rotate(glm::radians(glm::vec2(window.cursor_delta)), dt);
            }
            if (window.key[GLFW_KEY_P]) {
                cam.reset();
            }
        }
    }
}
