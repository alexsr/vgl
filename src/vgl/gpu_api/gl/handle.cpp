#include "handle.hpp"
#include <utility>

void vgl::gl::delete_buffer(GLuint id) {
    if (glIsBuffer(id)) {
        glDeleteBuffers(1, &id);
    }
}

void vgl::gl::delete_buffers(std::initializer_list<GLuint> const& ids) {
    for (auto const id : ids) {
        delete_buffer(id);
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

void vgl::gl::delete_textures(std::initializer_list<GLuint> const& ids) {
    for (auto const id : ids) {
        delete_texture(id);
    }
}

void vgl::gl::delete_shader(GLuint id) {
    if (glIsShader(id)) {
        glDeleteShader(id);
    }
}

void vgl::gl::delete_shaders(std::initializer_list<GLuint> const& ids) {
    for (auto const id : ids) {
        delete_shader(id);
    }
}

void vgl::gl::delete_program(GLuint id) {
    if (glIsProgram(id)) {
        glDeleteProgram(id);
    }
}

void vgl::gl::delete_programs(std::initializer_list<GLuint> const& ids) {
    for (auto const id : ids) {
        delete_program(id);
    }
}

void vgl::gl::delete_sampler(GLuint id) {
    if (glIsSampler(id)) {
        glDeleteSamplers(1, &id);
    }
}

void vgl::gl::delete_samplers(std::initializer_list<GLuint> const& ids) {
    for (auto const id : ids) {
        delete_sampler(id);
    }
}

void vgl::gl::delete_framebuffer(GLuint id) {
    if (glIsFramebuffer(id)) {
        glDeleteFramebuffers(1, &id);
    }
}

void vgl::gl::delete_framebuffers(std::initializer_list<GLuint> const& ids) {
    for (auto const id : ids) {
        delete_framebuffer(id);
    }
}

void vgl::gl::delete_vertex_array(GLuint id) {
    if (glIsVertexArray(id)) {
        glDeleteVertexArrays(1, &id);
    }
}

void vgl::gl::delete_vertex_arrays(std::initializer_list<GLuint> const& ids) {
    for (auto const id : ids) {
        delete_vertex_array(id);
    }
}

void vgl::gl::delete_query(GLuint id) {
    if (glIsQuery(id)) {
        glDeleteQueries(1, &id);
    }
}

void vgl::gl::delete_queries(std::initializer_list<GLuint> const& ids) {
    for (auto const id : ids) {
        delete_query(id);
    }
}
