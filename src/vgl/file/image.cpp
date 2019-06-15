#include "image.hpp"

vgl::Image_desc vgl::img::get_image_desc(const stbi_variant& stbi) {
    return std::visit([](auto&& img) {
        return img.desc;
        }, stbi);
}

vgl::image_type vgl::img::get_image_type(const stbi_variant& stbi) {
    return std::visit([](auto&& img) {
        return img.get_image_type();
        }, stbi);
}
