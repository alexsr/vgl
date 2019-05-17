#pragma once

#include "handle.hpp"
#include "vgl/utility/type_checking.hpp"
#include "vgl/utility/size_functions.hpp"

namespace vgl::gl
{
    template <typename T>
    std::enable_if_t<is_container_v<T>> set_buffer_storage(const glbuffer& buffer, const T& data, GLenum bitfield = 0) {
        glNamedBufferStorage(buffer, std::size(data) * sizeof_value_type(data), std::data(data), bitfield);
    }

    template <typename T>
    std::enable_if_t<!is_container_v<T>> set_buffer_storage(const glbuffer& buffer, const T& data, GLenum bitfield = 0) {
        glNamedBufferStorage(buffer, sizeof(T), &data, bitfield);
    }

    void set_empty_buffer_storage(const glbuffer& buffer, size_t size, GLenum bitfield = 0) {
        glNamedBufferStorage(buffer, size, nullptr, bitfield);
    }

    template <typename T>
    std::enable_if_t<is_container_v<T>> update_full_buffer(const glbuffer& buffer, const T& data) {
        const auto buffer_ptr = glMapNamedBufferRange(buffer, 0, std::size(data) * sizeof_value_type(data),
            GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
        std::memcpy(buffer_ptr, data.data(), std::size(data) * sizeof_value_type(data));
        glUnmapNamedBuffer(buffer);
    }

    template <typename T>
    std::enable_if_t<!is_container_v<T>> update_full_buffer(const glbuffer& buffer, const T& data) {
        const auto buffer_ptr = glMapNamedBufferRange(buffer, 0, sizeof(T),
            GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
        std::memcpy(buffer_ptr, &data, sizeof(T));
        glUnmapNamedBuffer(buffer);
    }

    template <typename T>
    glbuffer create_buffer(T data, GLenum bitfield = 0) {
        glbuffer handle = create_buffer();
        set_buffer_storage(handle, data, bitfield);
        return handle;
    }

    template <typename T>
    glbuffer create_buffer_fixed_size(T data, size_t size, GLenum bitfield = 0) {
        glbuffer handle = create_buffer();
        set_buffer_storage(handle, data, bitfield);
        return handle;
    }

    glbuffer create_buffer() {
        GLuint buffer = 0;
        glCreateBuffers(1, &buffer);
        return buffer;
    }
}
