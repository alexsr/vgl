#pragma once

#include "handle.hpp"
#include "vgl/file/image_file.hpp"
#include <glad/glad.h>

namespace vgl::gl
{
    inline gltexture create_texture(GLenum target) {
        GLuint texture = 0;
        glCreateTextures(target, 1, &texture);
        return texture;
    }

    inline GLuint64 get_texture_handle(const gltexture& texture) {
        const auto handle = glGetTextureHandleARB(texture);
        if (!glIsTextureHandleResidentARB(handle)) {
            glMakeTextureHandleResidentARB(handle);
        }
        return handle;
    }
    inline bool set_texture_data_2d(const gltexture& texture, const Image& image, int level = 0) {
        if (std::holds_alternative<stbi_uc*>(image.ptr)) {
            auto ptr = std::get<stbi_uc*>(image.ptr);
            glTextureSubImage2D(texture, level, 0, 0, image.desc.image_size.x, image.desc.image_size.y,
                image.desc.format, GL_UNSIGNED_BYTE, ptr);
            stbi_image_free(ptr);
            return true;
        }
        else if (std::holds_alternative<stbi_us*>(image.ptr)) {
            auto ptr = std::get<stbi_us*>(image.ptr);
            glTextureSubImage2D(texture, level, 0, 0, image.desc.image_size.x, image.desc.image_size.y,
                image.desc.format, GL_UNSIGNED_SHORT, ptr);
            stbi_image_free(ptr);
            return true;
        }
        else if (std::holds_alternative<float*>(image.ptr)) {
            auto ptr = std::get<float*>(image.ptr);
            glTextureSubImage2D(texture, level, 0, 0, image.desc.image_size.x, image.desc.image_size.y,
                image.desc.format, GL_FLOAT, ptr);
            stbi_image_free(ptr);
            return true;
        }
        return false;
    }
    inline bool set_cubemap_data(const gltexture& texture, const Image& image, int face) {
        if (std::holds_alternative<stbi_uc*>(image.ptr)) {
            auto ptr = std::get<stbi_uc*>(image.ptr);
            glTextureSubImage3D(texture, 0, 0, 0, face, image.desc.image_size.x, image.desc.image_size.y, 1,
                image.desc.format, GL_UNSIGNED_BYTE, ptr);
            stbi_image_free(ptr);
            return true;
        }
        else if (std::holds_alternative<stbi_us*>(image.ptr)) {
            auto ptr = std::get<stbi_us*>(image.ptr);
            glTextureSubImage3D(texture, 0, 0, 0, face, image.desc.image_size.x, image.desc.image_size.y, 1,
                image.desc.format, GL_UNSIGNED_SHORT, ptr);
            stbi_image_free(ptr);
            return true;
        }
        else if (std::holds_alternative<float*>(image.ptr)) {
            auto ptr = std::get<float*>(image.ptr);
            glTextureSubImage3D(texture, 0, 0, 0, face, image.desc.image_size.x, image.desc.image_size.y, 1,
                image.desc.format, GL_FLOAT, ptr);
            stbi_image_free(ptr);
            return true;
        }
        return false;
    }
}
