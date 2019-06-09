#pragma once

#include "glm/vec2.hpp"
#include <vector>
#include <variant>
#include <stb/stb_image.h>
#include <filesystem>

namespace vgl {
    struct Image_info {
        std::filesystem::path file_path;
        int channels{};
    };

    struct Image_desc {
        glm::ivec2 image_size;
        unsigned int format;
        unsigned int internal_format;
        unsigned int type;
    };

    struct Image {
        std::variant<stbi_uc*, stbi_us*, float*> ptr;
        Image_desc desc;
    };

    namespace file {
        Image_desc retrieve_image_desc(const std::filesystem::path& file_path, int channels = 4);
        Image_desc retrieve_image_desc(const Image_info& tex_info);
        Image load_image(const std::filesystem::path& file_path, int channels = 4, bool flip = false);
        Image load_image(const Image_info& tex_info, bool flip = false);
        std::vector<Image> load_images(const std::vector<Image_info>& textures);
    }
}
