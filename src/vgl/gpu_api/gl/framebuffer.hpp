#pragma once

#include "handle.hpp"

namespace vgl::gl {
    void check_framebuffer(const glframebuffer& framebuffer) {
        if (glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error{ "Framebuffer " + std::to_string(framebuffer.id()) + " was not created." };
        }
    }
    glframebuffer create_framebuffer() {
        GLuint framebuffer = 0;
        glCreateFramebuffers(1, &framebuffer);
        return glframebuffer(framebuffer);
    }
}
