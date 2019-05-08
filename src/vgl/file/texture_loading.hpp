#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "glm/glm.hpp"
#include <vector>
#include <variant>
#include <filesystem>
#include "vgl/rendering/tex_info.hpp"

namespace vgl {
    template <typename T>
    struct Tex_def {
        T* ptr;
        glm::ivec2 image_size;
        GLenum format;
        GLenum internal_format;
    };

    std::vector<std::variant<Tex_def<stbi_uc>, Tex_def<stbi_us>, Tex_def<float>>>
        load_texture_files(const std::vector<Texture_info>& textures) {
        std::vector<std::variant<Tex_def<stbi_uc>, Tex_def<stbi_us>, Tex_def<float>>> tex_data(textures.size());
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(textures.size()); ++i) {
            auto image_path = textures.at(i).file_path.string();
            auto required_channels = textures.at(i).channels;
            auto image_channels = textures.at(i).channels;
            if (stbi_is_hdr(image_path.c_str())) {
                Tex_def<float> def;
                stbi_info(image_path.c_str(), &def.image_size.x, &def.image_size.y, nullptr);
                stbi_set_flip_vertically_on_load(1);
                def.ptr = stbi_loadf(image_path.c_str(), &def.image_size.x, &def.image_size.y, &image_channels, required_channels);
                auto format = gen_tex_format(required_channels, GL_FLOAT);
                def.format = format.format;
                def.internal_format = format.internal_format;
                tex_data.at(i) = def;
            }
            else if (stbi_is_16_bit(image_path.c_str())) {
                Tex_def<stbi_us> def;
                stbi_info(image_path.c_str(), &def.image_size.x, &def.image_size.y, nullptr);
                stbi_set_flip_vertically_on_load(1);
                def.ptr = stbi_load_16(image_path.c_str(), &def.image_size.x, &def.image_size.y, &image_channels, required_channels);
                auto format = gen_tex_format(required_channels, GL_UNSIGNED_SHORT);
                def.format = format.format;
                def.internal_format = format.internal_format;
                tex_data.at(i) = def;
            }
            else {
                Tex_def<stbi_uc> def;
                stbi_info(image_path.c_str(), &def.image_size.x, &def.image_size.y, nullptr);
                stbi_set_flip_vertically_on_load(1);
                def.ptr = stbi_load(image_path.c_str(), &def.image_size.x, &def.image_size.y, &image_channels, required_channels);
                auto format = gen_tex_format(required_channels, GL_UNSIGNED_BYTE);
                def.format = format.format;
                def.internal_format = format.internal_format;
                tex_data.at(i) = def;
            }
        }
        return tex_data;
    }
}
