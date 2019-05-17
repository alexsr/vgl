#pragma once

#include "glad/glad.h"

namespace vgl {
    struct Texture_info {
        std::filesystem::path file_path;
        int channels{};
    };

    struct Texture_format {
        GLenum format;
        GLenum internal_format;
    };

    inline Texture_format gen_tex_format(int channels, GLenum type) {
        Texture_format f{};
        switch (channels) {
        case 1:
            f.format = GL_RED;
            switch (type) {
            case GL_FLOAT:
                f.internal_format = GL_R32F;
                break;
            case GL_UNSIGNED_SHORT:
                f.internal_format = GL_R16UI;
                break;
            default:
                f.internal_format = GL_R8;
                break;
            }
            break;
        case 2:
            f.format = GL_RG;
            switch (type) {
            case GL_FLOAT:
                f.internal_format = GL_RG32F;
                break;
            case GL_UNSIGNED_SHORT:
                f.internal_format = GL_RG16UI;
                break;
            default:
                f.internal_format = GL_RG8;
                break;
            }
            break;
        case 3:
            f.format = GL_RGB;
            switch (type) {
            case GL_FLOAT:
                f.internal_format = GL_RGB32F;
                break;
            case GL_UNSIGNED_SHORT:
                f.internal_format = GL_RGB16UI;
                break;
            default:
                f.internal_format = GL_RGB8;
                break;
            }
            break;
        default:
            f.format = GL_RGBA;
            switch (type) {
            case GL_FLOAT:
                f.internal_format = GL_RGBA32F;
                break;
            case GL_UNSIGNED_SHORT:
                f.internal_format = GL_RGBA16UI;
                break;
            default:
                f.internal_format = GL_RGBA8;
                break;
            }
            break;
        }
        return f;
    }
}
