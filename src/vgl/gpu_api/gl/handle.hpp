#pragma once

#include <glad/glad.h>
#include <utility>

namespace vgl::gl {
    namespace impl {
        typedef void (*handle_del)(GLuint);

        template <handle_del D>
        class handle {
        public:
            handle() : _id(0) {}
            handle(GLuint id) : _id(id) {}
            handle(const handle& h) = delete;
            handle(handle&& h) noexcept : _id(std::exchange(h._id, 0)) {}
            handle& operator=(const handle& h) = delete;
            handle& operator=(handle&& h) noexcept {
                std::swap(_id, h._id);
                return *this;
            }
            operator GLuint() const {
                return _id;
            }
            handle& operator=(GLuint id) {
                D(_id);
                _id = id;
                return *this;
            }
            GLuint* operator&() {
                return &_id;
            }
            GLuint id() const {
                return _id;
            }
            void reset() {
                D(_id);
                _id = 0;
            }
            ~handle() {
                D(_id);
            }
        private:
            GLuint _id;
        };
    }
    void delete_buffer(GLuint id) {
        if (glIsBuffer(id)) {
            glDeleteBuffers(1, &id);
        }
    }
    void delete_texture(GLuint id) {
        if (glIsTexture(id)) {
            const auto handle = glGetTextureHandleARB(id);
            if (glIsTextureHandleResidentARB(handle)) {
                glMakeTextureHandleNonResidentARB(handle);
            }
            glDeleteTextures(1, &id);
        }
    }
    void delete_shader(GLuint id) {
        if (glIsShader(id)) {
            glDeleteShader(id);
        }
    }
    void delete_program(GLuint id) {
        if (glIsProgram(id)) {
            glDeleteProgram(id);
        }
    }
    void delete_sampler(GLuint id) {
        if (glIsSampler(id)) {
            glDeleteSamplers(1, &id);
        }
    }
    void delete_framebuffer(GLuint id) {
        if (glIsFramebuffer(id)) {
            glDeleteFramebuffers(1, &id);
        }
    }
    void delete_vertex_array(GLuint id) {
        if (glIsVertexArray(id)) {
            glDeleteVertexArrays(1, &id);
        }
    }
    void delete_query(GLuint id) {
        if (glIsQuery(id)) {
            glDeleteQueries(1, &id);
        }
    }
    using glbuffer = impl::handle<delete_buffer>;
    using gltexture = impl::handle<delete_texture>;
    using glprogram = impl::handle<delete_program>;
    using glshader = impl::handle<delete_shader>;
    using glsampler = impl::handle<delete_sampler>;
    using glframebuffer = impl::handle<delete_framebuffer>;
    using glvertexarray = impl::handle<delete_vertex_array>;
    using glquery = impl::handle<delete_query>;

    template<void (*handle_del)(GLuint)>
    void delete_n(std::initializer_list<GLuint> handles) {
        for (auto& h : handles) {
            handle_del(h);
        }
    }
}
