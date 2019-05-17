#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "glm/glm.hpp"
#include <vector>
#include <variant>
#include <filesystem>
#include "vgl/rendering/tex_info.hpp"

namespace vgl {
    struct Tex_def {
        glm::ivec2 image_size;
        GLenum format;
        GLenum internal_format;
        GLenum type;
    };
    struct Tex_data {
        std::variant<stbi_uc*, stbi_us*, float*> ptr;
        Tex_def def;
    };
    namespace file {
        Tex_def load_tex_def(const Texture_info& tex_info) {
            Tex_def def;
            def.type = GL_UNSIGNED_BYTE;
            auto path = tex_info.file_path.string();
            if (stbi_is_hdr(path.c_str())) {
                def.type = GL_FLOAT;
            }
            else if (stbi_is_16_bit(path.c_str())) {
                def.type = GL_UNSIGNED_SHORT;
            }
            auto format = gen_tex_format(tex_info.channels, def.type);
            def.format = format.format;
            def.internal_format = format.internal_format;
            stbi_info(path.c_str(), &def.image_size.x, &def.image_size.y, nullptr);
            return def;
        }

        Tex_data load_texture(const Texture_info& tex_info, bool flip = false) {
            auto image_path = tex_info.file_path.string();
            auto required_channels = tex_info.channels;
            auto image_channels = tex_info.channels;
            Tex_data data;
            data.def = load_tex_def(tex_info);
            if (flip) {
                stbi_set_flip_vertically_on_load(1);
            }
            if (stbi_is_hdr(image_path.c_str())) {
                data.ptr = stbi_loadf(image_path.c_str(), &data.def.image_size.x, &data.def.image_size.y,
                    &image_channels, required_channels);
            }
            else if (stbi_is_16_bit(image_path.c_str())) {
                data.ptr = stbi_load_16(image_path.c_str(), &data.def.image_size.x, &data.def.image_size.y,
                    &image_channels, required_channels);
            }
            else {
                data.ptr = stbi_load(image_path.c_str(), &data.def.image_size.x, &data.def.image_size.y,
                    &image_channels, required_channels);
            }
            return data;
        }

        std::vector<Tex_data> load_textures(const std::vector<Texture_info>& textures) {
            std::vector<Tex_data> tex_data(textures.size());
#pragma omp parallel for
            for (int i = 0; i < static_cast<int>(textures.size()); ++i) {
                tex_data.at(i) = load_texture(textures.at(i));
            }
            return tex_data;
        }
    }
}
