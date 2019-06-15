#pragma once

#include <vector>
#include <variant>

namespace vgl {
    enum class image_type {
        ubyte,
        ushort,
        hdr
    };

    template <typename T>
    constexpr image_type convert_to_img_type() {
        if constexpr (std::is_same_v<T, float>) {
            return image_type::hdr;
        }
        else if constexpr (std::is_same_v<T, unsigned short>) {
            return image_type::ushort;
        }
        else if constexpr (std::is_same_v<T, unsigned char>) {
            return image_type::ubyte;
        }
        return image_type::ubyte;
    }

    struct Image_desc {
        int width;
        int height;
        int channels;
    };

    template <typename T>
    struct Image {
        Image() = default;
        explicit Image(Image_desc d) noexcept : desc(d) {
            data = std::vector<T>(d.width * d.height * d.channels);
        }
        Image(Image_desc d, T* ptr) : desc(d) {
            data = std::vector<T>(ptr, ptr + desc.width * desc.height * desc.channels);
            delete ptr;
        }
        using value_type = T;
        std::vector<T> data;
        Image_desc desc;
        constexpr image_type get_image_type() const {
            return convert_to_img_type<T>();
        }
    };

    using Image_f = Image<float>;
    using Image_uc = Image<unsigned char>;
    using Image_us = Image<unsigned short>;

    using stbi_variant = std::variant<Image_uc, Image_us, Image_f>;

    namespace img {
        constexpr size_t size_of(image_type t) {
            switch (t) {
            case image_type::ushort:
                return sizeof(unsigned short);
            case image_type::hdr:
                return sizeof(float);
            case image_type::ubyte:
            default:
                return sizeof(unsigned char);
            }
        }
        Image_desc get_image_desc(const stbi_variant& stbi);
        image_type get_image_type(const stbi_variant& stbi);
    }
}