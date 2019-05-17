#pragma once

#include <utility>
#include <glad/glad.h>

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
            inline handle& operator=(handle&& h) noexcept {
                std::swap(_id, h._id);
                return *this;
            }
            inline operator GLuint() const {
                return _id;
            }
            inline handle& operator=(GLuint id) {
                D(_id);
                _id = id;
                return *this;
            }
            inline GLuint* operator&() {
                return &_id;
            }
            inline GLuint id() const {
                return _id;
            }
            inline void reset() {
                D(_id);
                _id = 0;
            }
            inline ~handle() {
                D(_id);
            }
        private:
            GLuint _id;
        };
    }

    void delete_buffer(GLuint id);
    void delete_texture(GLuint id);
    void delete_shader(GLuint id);
    void delete_program(GLuint id);
    void delete_sampler(GLuint id);
    void delete_framebuffer(GLuint id);
    void delete_vertex_array(GLuint id);
    void delete_query(GLuint id);
    using glbuffer = impl::handle<delete_buffer>;
    using gltexture = impl::handle<delete_texture>;
    using glprogram = impl::handle<delete_program>;
    using glshader = impl::handle<delete_shader>;
    using glsampler = impl::handle<delete_sampler>;
    using glframebuffer = impl::handle<delete_framebuffer>;
    using glvertexarray = impl::handle<delete_vertex_array>;
    using glquery = impl::handle<delete_query>;
}
