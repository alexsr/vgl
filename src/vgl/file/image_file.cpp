#include "image_file.hpp"
#include <filesystem>
#include "stb/stb_image.h"
#include "vgl/gpu_api/gl/tex_format.hpp"

vgl::Image_desc vgl::file::retrieve_image_desc(const std::filesystem::path& file_path, int channels) {
    Image_desc desc;
    desc.type = GL_UNSIGNED_BYTE;
    auto path = file_path.string();
    if (stbi_is_hdr(path.c_str())) {
        desc.type = GL_FLOAT;
    }
    else if (stbi_is_16_bit(path.c_str())) {
        desc.type = GL_UNSIGNED_SHORT;
    }
    auto format = gl::gen_tex_format(channels, desc.type);
    desc.format = format.format;
    desc.internal_format = format.internal_format;
    stbi_info(path.c_str(), &desc.image_size.x, &desc.image_size.y, nullptr);
    return desc;
}

vgl::Image_desc vgl::file::retrieve_image_desc(const vgl::Image_info& tex_info) {
    return retrieve_image_desc(tex_info.file_path, tex_info.channels);
}

vgl::Image vgl::file::load_image(const std::filesystem::path& file_path, int channels, bool flip) {
    auto image_path = file_path.string();
    auto required_channels = channels;
    auto image_channels = channels;
    Image data;
    data.desc = retrieve_image_desc(file_path, channels);
    if (flip) {
        stbi_set_flip_vertically_on_load(1);
    }
    if (stbi_is_hdr(image_path.c_str())) {
        data.ptr = stbi_loadf(image_path.c_str(), &data.desc.image_size.x, &data.desc.image_size.y,
            &image_channels, required_channels);
    }
    else if (stbi_is_16_bit(image_path.c_str())) {
        data.ptr = stbi_load_16(image_path.c_str(), &data.desc.image_size.x, &data.desc.image_size.y,
            &image_channels, required_channels);
    }
    else {
        data.ptr = stbi_load(image_path.c_str(), &data.desc.image_size.x, &data.desc.image_size.y,
            &image_channels, required_channels);
    }
    if (flip) {
        stbi_set_flip_vertically_on_load(0);
    }
    return data;
}

vgl::Image vgl::file::load_image(const vgl::Image_info& tex_info, bool flip) {
    return load_image(tex_info.file_path, tex_info.channels, flip);
}

std::vector<vgl::Image> vgl::file::load_images(const std::vector<vgl::Image_info>& textures) {
    std::vector<Image> tex_data(textures.size());
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(textures.size()); ++i) {
        tex_data.at(i) = load_image(textures.at(i));
    }
    return tex_data;
}
