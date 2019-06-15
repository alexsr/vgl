#pragma once

#include <vector>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include "image.hpp"
#include <filesystem>
#include "vgl/utility/finally.hpp"
#include <type_traits>

namespace vgl {
    struct Image_info {
        std::filesystem::path file_path;
        int channels{};
    };
    namespace file {
        Image_desc retrieve_image_desc(const std::filesystem::path& file_path, int channels = 0);
        Image_desc retrieve_image_desc(const Image_info& image_info);
        image_type retrieve_image_type(const std::filesystem::path& file_path);
        image_type retrieve_image_type(const Image_info& image_info);
        stbi_variant load_image(const std::filesystem::path& file_path, int channels = 4, bool flip = false);
        stbi_variant load_image(const Image_info& tex_info, bool flip = false);
        std::vector<stbi_variant> load_images(const std::vector<Image_info>& image_infos, bool flip);
        template <typename T>
        bool save_image(const Image<T>& img, const std::filesystem::path& file_path, bool flip = false);
    }

    template <typename T>
    bool vgl::file::save_image(const Image<T>& img, const std::filesystem::path& file_path, bool flip) {
        if (flip) {
            stbi_flip_vertically_on_write(1);
        }
        auto reset_flip = vgl::cpp::final_op([flip]() { if (flip) stbi_set_flip_vertically_on_load(0); });
        auto ext = file_path.extension().string();
        if (ext == ".png") {
            stbi_write_png(file_path.string().c_str(), img.desc.width, img.desc.height, img.desc.channels, img.data.data(), 0);
        }
        else if (ext == ".jpg") {
            stbi_write_jpg(file_path.string().c_str(), img.desc.width, img.desc.height, img.desc.channels, img.data.data(), 100);
        }
        else if (ext == ".bmp") {
            stbi_write_bmp(file_path.string().c_str(), img.desc.width, img.desc.height, img.desc.channels, img.data.data());
        }
        else if (ext == ".hdr") {
            if constexpr (std::is_same_v<T, float>) {
                stbi_write_hdr(file_path.string().c_str(), img.desc.width, img.desc.height, img.desc.channels, img.data.data());
            }
            else {
                return false;
            }
        }
        else if (ext == ".tga") {
            stbi_write_tga(file_path.string().c_str(), img.desc.width, img.desc.height, img.desc.channels, img.data.data());
        }
        else {
            return false;
        }
        return true;
    }
}
