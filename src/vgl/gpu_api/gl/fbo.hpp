#pragma once

namespace vgl::gl {
    template <typename... Args>
    GLuint create_framebuffer(Args&&... args) {
        GLuint framebuffer = 0;
        glCreateFramebuffers(1, &framebuffer);
        if constexpr (sizeof...(args) != 0) {
            std::vector<GLenum> attachments{ std::forward<Args>(args)... };
            glNamedFramebufferDrawBuffers(framebuffer, static_cast<GLsizei>(attachments.size()), attachments.data());
        }
        if (glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error{ "Framebuffer " + std::to_string(framebuffer) + " was not created." };
        }
        return framebuffer;
    }
}
