#include "handle.hpp"
#include <utility>

void vgl::gl::delete_buffer(GLuint id) {
    if (glIsBuffer(id)) {
        glDeleteBuffers(1, &id);
    }
}

void vgl::gl::delete_texture(GLuint id) {
    if (glIsTexture(id)) {
        const auto handle = glGetTextureHandleARB(id);
        if (glIsTextureHandleResidentARB(handle)) {
            glMakeTextureHandleNonResidentARB(handle);
        }
        glDeleteTextures(1, &id);
    }
}

void vgl::gl::delete_shader(GLuint id) {
    if (glIsShader(id)) {
        glDeleteShader(id);
    }
}

void vgl::gl::delete_program(GLuint id) {
    if (glIsProgram(id)) {
        glDeleteProgram(id);
    }
}

void vgl::gl::delete_sampler(GLuint id) {
    if (glIsSampler(id)) {
        glDeleteSamplers(1, &id);
    }
}

void vgl::gl::delete_framebuffer(GLuint id) {
    if (glIsFramebuffer(id)) {
        glDeleteFramebuffers(1, &id);
    }
}

void vgl::gl::delete_vertex_array(GLuint id) {
    if (glIsVertexArray(id)) {
        glDeleteVertexArrays(1, &id);
    }
}

void vgl::gl::delete_query(GLuint id) {
    if (glIsQuery(id)) {
        glDeleteQueries(1, &id);
    }
}
