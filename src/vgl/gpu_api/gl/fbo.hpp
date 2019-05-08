#pragma once

#include "glad/glad.h"

namespace vgl::gl {

    void check_framebuffer(GLuint framebuffer) {
        if (glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error{ "Framebuffer " + std::to_string(framebuffer) + " was not created." };
        }
    }

    GLuint create_framebuffer() {
        GLuint framebuffer = 0;
        glCreateFramebuffers(1, &framebuffer);
        return framebuffer;
    }

    void delete_framebuffer(GLuint fb) {
        if (glIsFramebuffer(fb)) {
            glDeleteFramebuffers(1, &fb);
        }
    }

    void attach_drawbuffers(GLuint framebuffer, std::initializer_list<GLenum> attachments) {
        glNamedFramebufferDrawBuffers(framebuffer, static_cast<GLsizei>(attachments.size()), std::data(attachments));
    }
}
