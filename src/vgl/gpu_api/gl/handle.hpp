#pragma once

#include <utility>
#include <glad/glad.h>

namespace vgl::gl {
    namespace impl {
        typedef void (*handle_del)(GLuint);

        template <handle_del D>
        class handle {
        public:
            handle() : id_(0) {}
            handle(GLuint id) : id_(id) {}
            handle(const handle& h) = delete;
            handle(handle&& h) noexcept : id_(std::exchange(h.id_, 0)) {}
            handle& operator=(const handle& h) = delete;
            inline handle& operator=(handle&& h) noexcept {
                std::swap(id_, h.id_);
                return *this;
            }
            inline operator GLuint() const {
                return id_;
            }
            inline handle& operator=(GLuint id) {
                D(id_);
                id_ = id;
                return *this;
            }
            inline GLuint* operator&() {
                return &id_;
            }
            inline GLuint id() const {
                return id_;
            }
            inline void reset() {
                D(id_);
                id_ = 0;
            }
            inline ~handle() {
                D(id_);
            }
        private:
            GLuint id_;
        };
    }

    void delete_buffer(GLuint id);
    void delete_buffers(std::initializer_list<GLuint> const& ids);
    void delete_texture(GLuint id);
    void delete_textures(std::initializer_list<GLuint> const& ids);
    void delete_shader(GLuint id);
    void delete_shaders(std::initializer_list<GLuint> const& ids);
    void delete_program(GLuint id);
    void delete_programs(std::initializer_list<GLuint> const& ids);
    void delete_sampler(GLuint id);
    void delete_samplers(std::initializer_list<GLuint> const& ids);
    void delete_framebuffer(GLuint id);
    void delete_framebuffers(std::initializer_list<GLuint> const& ids);
    void delete_vertex_array(GLuint id);
    void delete_vertex_arrays(std::initializer_list<GLuint> const& ids);
    void delete_query(GLuint id);
    void delete_queries(std::initializer_list<GLuint> const& ids);
    using glbuffer = impl::handle<delete_buffer>;
    using gltexture = impl::handle<delete_texture>;
    using glprogram = impl::handle<delete_program>;
    using glshader = impl::handle<delete_shader>;
    using glsampler = impl::handle<delete_sampler>;
    using glframebuffer = impl::handle<delete_framebuffer>;
    using glvertexarray = impl::handle<delete_vertex_array>;
    using glquery = impl::handle<delete_query>;
}
