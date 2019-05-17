#pragma once

#include "handle.hpp"

namespace vgl::gl {
    inline void check_framebuffer(const glframebuffer& framebuffer) {
        if (glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error{ "Framebuffer " + std::to_string(framebuffer.id()) + " was not created." };
        }
    }

    inline void attach_draw_buffers(const glframebuffer& framebuffer, std::initializer_list<GLenum> draw_buffers) {
        glNamedFramebufferDrawBuffers(framebuffer, static_cast<int>(std::size(draw_buffers)), std::data(draw_buffers));
    }

    inline glframebuffer create_framebuffer() {
        GLuint framebuffer = 0;
        glCreateFramebuffers(1, &framebuffer);
        return glframebuffer(framebuffer);
    }
}
