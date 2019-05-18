#include "image_file.hpp"
#include <filesystem>
#include "stb/stb_image.h"
#include "vgl/gpu_api/gl/tex_format.hpp"

vgl::Tex_def vgl::file::load_tex_def(const std::filesystem::path& file_path, int channels) {
    Tex_def def;
    def.type = GL_UNSIGNED_BYTE;
    auto path = file_path.string();
    if (stbi_is_hdr(path.c_str())) {
        def.type = GL_FLOAT;
    }
    else if (stbi_is_16_bit(path.c_str())) {
        def.type = GL_UNSIGNED_SHORT;
    }
    auto format = gl::gen_tex_format(channels, def.type);
    def.format = format.format;
    def.internal_format = format.internal_format;
    stbi_info(path.c_str(), &def.image_size.x, &def.image_size.y, nullptr);
    return def;
}

vgl::Tex_def vgl::file::load_tex_def(const vgl::Texture_info& tex_info) {
    return load_tex_def(tex_info.file_path, tex_info.channels);
}

vgl::Tex_data vgl::file::load_texture(const std::filesystem::path& file_path, int channels, bool flip) {
    auto image_path = file_path.string();
    auto required_channels = channels;
    auto image_channels = channels;
    Tex_data data;
    data.def = load_tex_def(file_path, channels);
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

vgl::Tex_data vgl::file::load_texture(const vgl::Texture_info& tex_info, bool flip) {
    return load_texture(tex_info.file_path, tex_info.channels, flip);
}

std::vector<vgl::Tex_data> vgl::file::load_textures(const std::vector<vgl::Texture_info>& textures) {
    std::vector<Tex_data> tex_data(textures.size());
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(textures.size()); ++i) {
        tex_data.at(i) = load_texture(textures.at(i));
    }
    return tex_data;
}
