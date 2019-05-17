#pragma once

#include "handle.hpp"
#include "vgl/file/image_file.hpp"

namespace vgl::gl
{
    gltexture create_texture(GLenum target) {
        GLuint texture = 0;
        glCreateTextures(target, 1, &texture);
        return texture;
    }

    GLuint64 get_texture_handle(const GLuint texture) {
        const auto handle = glGetTextureHandleARB(texture);
        if (!glIsTextureHandleResidentARB(handle)) {
            glMakeTextureHandleResidentARB(handle);
        }
        return handle;
    }
    bool set_texture_data_2d(const gltexture& texture, Tex_data tex_data, int level = 0) {
        if (std::holds_alternative<stbi_uc*>(tex_data.ptr)) {
            auto ptr = std::get<stbi_uc*>(tex_data.ptr);
            glTextureSubImage2D(texture, level, 0, 0, tex_data.def.image_size.x, tex_data.def.image_size.y,
                tex_data.def.format, GL_UNSIGNED_BYTE, ptr);
            stbi_image_free(ptr);
            return true;
        }
        else if (std::holds_alternative<stbi_us*>(tex_data.ptr)) {
            auto ptr = std::get<stbi_us*>(tex_data.ptr);
            glTextureSubImage2D(texture, level, 0, 0, tex_data.def.image_size.x, tex_data.def.image_size.y,
                tex_data.def.format, GL_UNSIGNED_SHORT, ptr);
            stbi_image_free(ptr);
            return true;
        }
        else if (std::holds_alternative<float*>(tex_data.ptr)) {
            auto ptr = std::get<float*>(tex_data.ptr);
            glTextureSubImage2D(texture, level, 0, 0, tex_data.def.image_size.x, tex_data.def.image_size.y,
                tex_data.def.format, GL_FLOAT, ptr);
            stbi_image_free(ptr);
            return true;
        }
        return false;
    }
    bool set_cubemap_data(const gltexture& texture, Tex_data tex_data, int face) {
        if (std::holds_alternative<stbi_uc*>(tex_data.ptr)) {
            auto ptr = std::get<stbi_uc*>(tex_data.ptr);
            glTextureSubImage3D(texture, 0, 0, 0, face, tex_data.def.image_size.x, tex_data.def.image_size.y, 1,
                tex_data.def.format, GL_UNSIGNED_BYTE, ptr);
            stbi_image_free(ptr);
            return true;
        }
        else if (std::holds_alternative<stbi_us*>(tex_data.ptr)) {
            auto ptr = std::get<stbi_us*>(tex_data.ptr);
            glTextureSubImage3D(texture, 0, 0, 0, face, tex_data.def.image_size.x, tex_data.def.image_size.y, 1,
                tex_data.def.format, GL_UNSIGNED_SHORT, ptr);
            stbi_image_free(ptr);
            return true;
        }
        else if (std::holds_alternative<float*>(tex_data.ptr)) {
            auto ptr = std::get<float*>(tex_data.ptr);
            glTextureSubImage3D(texture, 0, 0, 0, face, tex_data.def.image_size.x, tex_data.def.image_size.y, 1,
                tex_data.def.format, GL_FLOAT, ptr);
            stbi_image_free(ptr);
            return true;
        }
        return false;
    }
}
