#pragma once

#include "glad/glad.h"
#include <string>
#include <iostream>

namespace vgl::gl {
    void APIENTRY debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
        const GLchar* message, const void* userParam) {
        // ignore non-significant error/warning codes
        if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

        std::string source_msg;
        std::string type_msg;
        std::string severity_msg;
        switch (source) {
        case GL_DEBUG_SOURCE_API:
            source_msg = "API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source_msg = "window system";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source_msg = "shader compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source_msg = "third party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            source_msg = "application";
            break;
        default:
            source_msg = "other";
            break;
        }
        switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            type_msg = "error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            type_msg = "deprecated behavior";
            break;
        case GL_DEBUG_TYPE_MARKER:
            type_msg = "marker";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            type_msg = "performance";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            type_msg = "undefined behavior";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            type_msg = "portability";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            type_msg = "push group";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            type_msg = "pop group";
            break;
        default:
            type_msg = "other";
            break;
        }
        switch (severity) {
        case GL_DEBUG_SEVERITY_LOW:
            severity_msg = "low";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            severity_msg = "medium";
            break;
        case GL_DEBUG_SEVERITY_HIGH:
            severity_msg = "high";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
        default:
            severity_msg = "notification";
            break;
        }
        std::cout << "[DBG_MSG (" << id << ")]" << message << "\n";
        std::cout << "[SOURCE]" << source_msg << "\n";
        std::cout << "[TYPE]" << type_msg << "\n";
        std::cout << "[SEVERITY]" << severity_msg << "\n";
    }
}
