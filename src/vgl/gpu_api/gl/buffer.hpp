#pragma once

#include "glad/glad.h"
#include "vgl/utility/type_checking.hpp"
#include "vgl/utility/size_functions.hpp"

namespace vgl
{
    namespace gl
    {
        template <typename T>
        std::enable_if_t<is_container_v<T>> set_buffer_storage(GLuint buffer, T data, GLenum bitfield = 0) {
            glNamedBufferStorage(buffer, std::size(data) * sizeof_value_type(data), std::data(data), bitfield);
        }

        template <typename T>
        std::enable_if_t<!is_container_v<T>> set_buffer_storage(GLuint buffer, T data, GLenum bitfield = 0) {
            glNamedBufferStorage(buffer, sizeof data, &data, bitfield);
        }

        void set_empty_buffer_storage(GLuint buffer, size_t size) {
            glNamedBufferStorage(buffer, size, nullptr, 0);
        }

        template <typename T>
        GLuint create_buffer(T data, GLenum bitfield = 0) {
            GLuint buffer = 0;
            glCreateBuffers(1, &buffer);
            set_buffer_storage(buffer, data, bitfield);
            return buffer;
        }

        GLuint create_buffer() {
            GLuint buffer = 0;
            glCreateBuffers(1, &buffer);
            return buffer;
        }
    }
}
