#include "handle.hpp"
#include <glad/glad.h>
#include <utility>

void vgl::gl::delete_buffer(unsigned int id) {
    if (glIsBuffer(id)) {
        glDeleteBuffers(1, &id);
    }
}

void vgl::gl::delete_texture(unsigned int id) {
    if (glIsTexture(id)) {
        const auto handle = glGetTextureHandleARB(id);
        if (glIsTextureHandleResidentARB(handle)) {
            glMakeTextureHandleNonResidentARB(handle);
        }
        glDeleteTextures(1, &id);
    }
}

void vgl::gl::delete_shader(unsigned int id) {
    if (glIsShader(id)) {
        glDeleteShader(id);
    }
}

void vgl::gl::delete_program(unsigned int id) {
    if (glIsProgram(id)) {
        glDeleteProgram(id);
    }
}

void vgl::gl::delete_sampler(unsigned int id) {
    if (glIsSampler(id)) {
        glDeleteSamplers(1, &id);
    }
}

void vgl::gl::delete_framebuffer(unsigned int id) {
    if (glIsFramebuffer(id)) {
        glDeleteFramebuffers(1, &id);
    }
}

void vgl::gl::delete_vertex_array(unsigned int id) {
    if (glIsVertexArray(id)) {
        glDeleteVertexArrays(1, &id);
    }
}

void vgl::gl::delete_query(unsigned int id) {
    if (glIsQuery(id)) {
        glDeleteQueries(1, &id);
    }
}
