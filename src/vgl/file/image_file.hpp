#pragma once

#include "glm/vec2.hpp"
#include <vector>
#include <variant>
#include <stb/stb_image.h>
#include "vgl/rendering/tex_info.hpp"

namespace vgl {
    struct Tex_def {
        glm::ivec2 image_size;
        unsigned int format;
        unsigned int internal_format;
        unsigned int type;
    };

    struct Tex_data {
        std::variant<stbi_uc*, stbi_us*, float*> ptr;
        Tex_def def;
    };

    namespace file {
        Tex_def load_tex_def(const Texture_info& tex_info);
        Tex_data load_texture(const Texture_info& tex_info, bool flip = false);
        std::vector<Tex_data> load_textures(const std::vector<Texture_info>& textures);
    }
}
