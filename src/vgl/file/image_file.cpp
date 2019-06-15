#include "image_file.hpp"

vgl::Image_desc vgl::file::retrieve_image_desc(const std::filesystem::path& file_path, int channels) {
    Image_desc desc;
    stbi_info(file_path.string().c_str(), &desc.width, &desc.height, &desc.channels);
    if (channels != 0) {
        desc.channels = channels;
    }
    return desc;
}

vgl::Image_desc vgl::file::retrieve_image_desc(const Image_info& image_info) {
    return retrieve_image_desc(image_info.file_path, image_info.channels);
}

vgl::image_type vgl::file::retrieve_image_type(const std::filesystem::path& file_path) {
    auto path = file_path.string();
    if (stbi_is_hdr(path.c_str())) {
        return image_type::hdr;
    }
    else if (stbi_is_16_bit(path.c_str())) {
        return image_type::ushort;
    }
    return image_type::ubyte;
}

vgl::image_type vgl::file::retrieve_image_type(const vgl::Image_info& image_info) {
    return retrieve_image_type(image_info.file_path);
}

vgl::stbi_variant vgl::file::load_image(const std::filesystem::path& file_path, int channels, bool flip) {
    auto image_path = file_path.string();
    auto image_channels = channels;
    auto desc = retrieve_image_desc(file_path);
    desc.channels = channels;
    if (flip) {
        stbi_set_flip_vertically_on_load(1);
    }
    auto reset_flip = vgl::cpp::final_op([flip]() { if (flip) stbi_set_flip_vertically_on_load(0); });
    auto size_of_data = desc.width * desc.height * channels;
    switch (retrieve_image_type(file_path)) {
    case image_type::hdr: {
        return Image_f(desc, stbi_loadf(image_path.c_str(), &desc.width, &desc.height,
            &image_channels, channels));
    }
    case image_type::ushort: {
        return Image_us(desc, stbi_load_16(image_path.c_str(), &desc.width, &desc.height,
            &image_channels, channels));
    }
    case image_type::ubyte: {
    default:
        return Image_uc(desc, stbi_load(image_path.c_str(), &desc.width, &desc.height,
            &image_channels, channels));
    }
    }
}

vgl::stbi_variant vgl::file::load_image(const vgl::Image_info& tex_info, bool flip) {
    return load_image(tex_info.file_path, tex_info.channels, flip);
}

std::vector<vgl::stbi_variant> vgl::file::load_images(const std::vector<vgl::Image_info>& image_infos, bool flip) {
    std::vector<stbi_variant> images(image_infos.size());
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(image_infos.size()); ++i) {
        images.at(i) = load_image(image_infos.at(i), flip);
    }
    return images;
}
