#pragma once

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
    template <typename... Args>
    void attach_drawbuffers(GLuint framebuffer, Args... args) {
        if constexpr (sizeof...(args) != 0) {
            std::vector<GLenum> attachments{static_cast<GLenum>(std::forward<Args>(args))... };
            glNamedFramebufferDrawBuffers(framebuffer, static_cast<GLsizei>(attachments.size()), attachments.data());
        }
    }
}
