#pragma once

#include <utility>

namespace vgl::gl {
    namespace impl {
        typedef void (*handle_del)(unsigned int);

        template <handle_del D>
        class handle {
        public:
            handle() : _id(0) {}
            handle(unsigned int id) : _id(id) {}
            handle(const handle& h) = delete;
            handle(handle&& h) noexcept : _id(std::exchange(h._id, 0)) {}
            handle& operator=(const handle& h) = delete;
            inline handle& operator=(handle&& h) noexcept {
                std::swap(_id, h._id);
                return *this;
            }
            inline operator unsigned int() const {
                return _id;
            }
            inline handle& operator=(unsigned int id) {
                D(_id);
                _id = id;
                return *this;
            }
            inline unsigned int* operator&() {
                return &_id;
            }
            inline unsigned int id() const {
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
            unsigned int _id;
        };
    }

    void delete_buffer(unsigned int id);
    void delete_texture(unsigned int id);
    void delete_shader(unsigned int id);
    void delete_program(unsigned int id);
    void delete_sampler(unsigned int id);
    void delete_framebuffer(unsigned int id);
    void delete_vertex_array(unsigned int id);
    void delete_query(unsigned int id);
    using glbuffer = impl::handle<delete_buffer>;
    using gltexture = impl::handle<delete_texture>;
    using glprogram = impl::handle<delete_program>;
    using glshader = impl::handle<delete_shader>;
    using glsampler = impl::handle<delete_sampler>;
    using glframebuffer = impl::handle<delete_framebuffer>;
    using glvertexarray = impl::handle<delete_vertex_array>;
    using glquery = impl::handle<delete_query>;
}
