#pragma once

#include "handle.hpp"
#include "vgl/file/image.hpp"
#include <glad/glad.h>

namespace vgl::gl {
    struct Texture_format {
        GLenum format;
        GLenum internal_format;
        GLenum type;
    };

    constexpr GLenum glify_image_type(image_type t) {
        switch (t) {
        case image_type::hdr:
            return GL_FLOAT;
        case image_type::ushort:
            return GL_UNSIGNED_SHORT;
        case image_type::ubyte:
        default:
            return GL_UNSIGNED_BYTE;
        }
    }
    template <typename T>
    constexpr GLenum glify_type() {
        if constexpr (std::is_same_v<T, float>) {
            return GL_FLOAT;
        }
        else if constexpr (std::is_same_v<T, unsigned short>) {
            return GL_UNSIGNED_SHORT;
        }
        else if constexpr (std::is_same_v<T, unsigned char>) {
            return GL_UNSIGNED_BYTE;
        }
        else {
            return GL_INVALID_ENUM;
        }
    }

    constexpr GLenum glify_channels(int channels, bool is_int = false) {
        if (!is_int) {
            switch (channels) {
            case 1:
                return GL_RED;
            case 2:
                return GL_RG;
            case 3:
                return GL_RGB;
            default:
                return GL_RGBA;
            }
        }
        else {
            switch (channels) {
            case 1:
                return GL_RED_INTEGER;
            case 2:
                return GL_RG_INTEGER;
            case 3:
                return GL_RGB_INTEGER;
            default:
                return GL_RGBA_INTEGER;
            }
        }
    }

    template <typename T>
    constexpr GLenum derive_internal_format(Image_desc desc) {
        if constexpr (std::is_same_v<T, float>) {
            switch (desc.channels) {
            case 1:
                return GL_R32F;
            case 2:
                return GL_RG32F;
            case 3:
                return GL_RGB32F;
            case 4:
            default:
                return GL_RGBA32F;
            }
        }
        else if constexpr (std::is_same_v<T, unsigned short>) {
            switch (desc.channels) {
            case 1:
                return GL_R16UI;
            case 2:
                return GL_RG16UI;
            case 3:
                return GL_RGB16UI;
            case 4:
            default:
                return GL_RGBA16UI;
            }
        }
        else if constexpr (std::is_same_v<T, unsigned char>) {
            switch (desc.channels) {
            case 1:
                return GL_R8;
            case 2:
                return GL_RG8;
            case 3:
                return GL_RGB8;
            case 4:
            default:
                return GL_RGBA8;
            }
        }
        else {
            return GL_INVALID_ENUM;
        }
    }

    constexpr GLenum derive_internal_format(Image_desc desc, image_type type) {
        switch (type) {
        case image_type::hdr:
            return derive_internal_format<float>(desc);
        case image_type::ushort:
            return derive_internal_format<unsigned short>(desc);
        default:
            return derive_internal_format<unsigned char>(desc);
        }
    }

    constexpr GLenum derive_internal_format(const stbi_variant& img) {
        return std::visit([](auto&& img) {
            return derive_internal_format<typename std::remove_reference_t<decltype(img)>::value_type>(img.desc);
        }, img);
    }

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

    template <typename T>
    bool set_texture_data_2d(const gltexture& texture, const std::vector<T>& data,
        int width, int height, GLenum format, int level = 0) {
        glTextureSubImage2D(texture, level, 0, 0, width, height,
            format, glify_type<T>(), data.data());
        return true;
    }

    template <typename T>
    bool set_texture_data_2d(const gltexture& texture, const Image<T>& img, int level = 0) {
        auto format = glify_channels(img.desc.channels);
        glTextureSubImage2D(texture, level, 0, 0, img.desc.width, img.desc.height,
            format, glify_type<T>(), img.data.data());
        return true;
    }

    inline bool set_texture_data_2d(const gltexture& texture, const stbi_variant& image, int level = 0) {
        if (image.valueless_by_exception()) {
            return false;
        }
        else {
            return std::visit([&texture, &level](auto&& img) {
                return set_texture_data_2d(texture, img, level);
                }, image);
        }
    }

    template <typename T>
    bool set_cubemap_data(const gltexture& texture, const Image<T>& img, int face) {
        auto format = glify_channels(img.desc.channels);
        glTextureSubImage3D(texture, 0, 0, 0, face, img.desc.width, img.desc.height, 1,
            format, glify_type<T>(), img.data.data());
        return true;
    }

    inline bool set_cubemap_data(const gltexture& texture, const stbi_variant& image, int face) {
        if (image.valueless_by_exception()) {
            return false;
        }
        else {
            return std::visit([&texture, &face](auto&& img) {
                return set_cubemap_data(texture, img, face);
                }, image);
        }
    }
}
