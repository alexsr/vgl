#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <cmath>
#include <corecrt_math_defines.h>

struct color4 {
    float r;
    float g;
    float b;
    float a;
};

color4 lookup(double x, double y) {
    return color4{};
}

struct vec2 {
    double x;
    double y;

    constexpr vec2 operator*(const double v) const {
        return vec2{x * v, y * v};
    }

    constexpr vec2 operator/(const double v) const {
        return vec2{x / v, y / v};
    }

    constexpr vec2 operator+(const vec2& v) const {
        return vec2{x + v.x, y + v.y};
    }

    constexpr vec2 operator-(const vec2& v) const {
        return vec2{x - v.x, y - v.y};
    }
};

struct vec3 {
    double x;
    double y;
    double z;

    constexpr vec3 operator*(const double v) const {
        return vec3{x * v, y * v, z * v};
    }

    constexpr vec3 operator/(const double v) const {
        return vec3{x / v, y / v, z / v};
    }

    constexpr vec3 operator+(const vec3& v) const {
        return vec3{x + v.x, y + v.y, z + v.z};
    }

    constexpr vec3 operator-(const vec3& v) const {
        return vec3{x - v.x, y - v.y, z - v.z};
    }
};

vec3 rotate_y(const vec3& v, double angle) {
    return vec3{ cos(angle) * v.x + sin(angle) * v.z,
    v.y, -sin(angle) * v.x + cos(angle) * v.z };
}

constexpr vec3 operator*(const double a, const vec3& v) {
    return vec3{v.x * a, v.y * a, v.z * a};
}

constexpr vec3 operator/(const double a, const vec3& v) {
    return vec3{v.x / a, v.y / a, v.z / a};
}

double dot(const vec3& a, const vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 normalize(const vec3& c) {
    return c / sqrt(dot(c, c));
}

vec3 map_to_unit_cube(const vec2& uv, vec3 top_left, vec3 down, vec3 right) {
    return uv.x * right + uv.y * down + top_left;
}

vec2 map_unit_cube_to_unit_sphere(const vec3& c) {
    const auto v = normalize(c);
    return vec2{atan2(v.z, v.x) + M_PI, acos(v.y)};
}

vec2 map_unit_sphere_to_equirectangular(const vec2& c) {
    auto x = c.x / M_PI;
    return vec2{x, c.y / M_PI};
}

template <typename T>
std::vector<T> populate_image(const std::vector<T>& ref, int ref_channels, int ref_height,
                                      int longitude_zero_degrees, vec3 top_left, vec3 down, vec3 right) {
    auto size = ref_height / 2;
    auto ref_width = 2 * ref_height;
    double pixel_dx = 1.0 / static_cast<double>(size - 1);
    double pixel_dy = 1.0 / static_cast<double>(size - 1);
    auto longitude_zero = longitude_zero_degrees % 360 / 180.0 * M_PI;
    top_left = rotate_y(top_left, longitude_zero);
    down = rotate_y(down, longitude_zero);
    right = rotate_y(right, longitude_zero);
    std::vector<T> image(ref.size());
    for (int j = 0; j < size; j++) {
        double v = j * pixel_dy + 0.5 * pixel_dy;
        for (int i = 0; i < size; i++) {
            double u = i * pixel_dx + 0.5 * pixel_dx;
            auto cube_coord = map_to_unit_cube({u, v}, top_left, down, right);
            auto sphere_coord = map_unit_cube_to_unit_sphere(cube_coord);
            auto equirect_coord = map_unit_sphere_to_equirectangular(sphere_coord);
            auto x = static_cast<int>(equirect_coord.x * ref_height) % ref_width;
            auto y = static_cast<int>(equirect_coord.y * ref_height) % ref_height;
            auto ref_id = y * ref_width * ref_channels + x * ref_channels;
            auto image_id = j * size * ref_channels + i * ref_channels;
            for (int k = 0; k < ref_channels; k++) {
                image.at(image_id + k) = ref.at(ref_id + k);
            }
        }
    }
    return image;
}

int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3) {
        std::cout << "Usage: cubemap_generator <path_to_image> [<longitude_zero>]\n";
        return 1;
    }
    const auto path = std::filesystem::path(argv[1]);
    if (!std::filesystem::exists(path)) {
        std::cout << "File not found.\n";
        return 1;
    }
    if (std::filesystem::is_directory(path)) {
        std::cout << "Path has to point to file.\n";
        return 1;
    }
    int longitude_zero_degrees = 0;
    if (argc == 3) {
        const std::string arg(argv[2]);
        try {
            std::size_t pos;
            longitude_zero_degrees = std::stoi(arg, &pos);
        }
        catch (std::invalid_argument const&) {
            std::cout << "Invalid number: " << arg << '\n';
        }
        catch (std::out_of_range const&) {
            std::cout << "Number out of range: " << arg << '\n';
        }
    }
    std::cout << "Longitude zero of " << longitude_zero_degrees << " degrees.";
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_info(path.string().c_str(), &width, &height, &channels);
    if (width == 0 || height == 0 || channels == 0) {
        std::cout << "Image is not valid.";
        return 1;
    }
    if (width != 2 * height) {
        std::cout << "Width has to be equal to 2 * height.";
        return 1;
    }

    auto out_path = path.parent_path() / path.filename().stem() / path.filename().stem();
    if (stbi_is_hdr(path.string().c_str())) {
        std::vector<float> image(width * height * channels);
        auto data = stbi_loadf(path.string().c_str(), &width, &height, &channels, channels);
        std::memcpy(image.data(), data, width * height * channels * sizeof(float));

        std::cout << "Writing to " << out_path << "\n";

        auto test = populate_image(image, channels, height, longitude_zero_degrees, { 1.0, 1.0, 1.0 }, { 0.0, -2.0, 0.0 },
            { -2.0, 0.0, 0.0 });
        stbi_write_hdr((out_path.string() + "_px.hdr").c_str(), height / 2, height / 2, channels, test.data());

        test = populate_image(image, channels, height, longitude_zero_degrees, { -1.0, 1.0, -1.0 }, { 2.0, 0.0, 0.0 },
            { 0.0, 0.0, 2.0 });
        stbi_write_hdr((out_path.string() + "_py.hdr").c_str(), height / 2, height / 2, channels, test.data());

        test = populate_image(image, channels, height, longitude_zero_degrees, { 1.0, 1.0, -1.0 }, { 0.0, -2.0, 0.0 },
            { 0.0, 0.0, 2.0 });
        stbi_write_hdr((out_path.string() + "_pz.hdr").c_str(), height / 2, height / 2, channels, test.data());

        test = populate_image(image, channels, height, longitude_zero_degrees, { -1.0, 1.0, -1.0 }, { 0.0, -2.0, 0.0 },
            { 2.0, 0.0, 0.0 });
        stbi_write_hdr((out_path.string() + "_nx.hdr").c_str(), height / 2, height / 2, channels, test.data());

        test = populate_image(image, channels, height, longitude_zero_degrees, { 1.0, -1.0, -1.0 }, { -2.0, 0.0, 0.0 },
            { 0.0, 0.0, 2.0 });
        stbi_write_hdr((out_path.string() + "_ny.hdr").c_str(), height / 2, height / 2, channels, test.data());

        test = populate_image(image, channels, height, longitude_zero_degrees, { -1.0, 1.0, 1.0 }, { 0.0, -2.0, 0.0 },
            { 0.0, 0.0, -2.0 });
        stbi_write_hdr((out_path.string() + "_nz.hdr").c_str(), height / 2, height / 2, channels, test.data());
    }
    else {
        std::vector<std::byte> image(width * height * channels);
        auto data = stbi_load(path.string().c_str(), &width, &height, &channels, channels);
        std::memcpy(image.data(), data, width * height * channels * sizeof(std::byte));

        std::cout << "Writing to " << out_path << "\n";

        auto test = populate_image(image, channels, height, longitude_zero_degrees, { 1.0, 1.0, 1.0 }, { 0.0, -2.0, 0.0 },
            { -2.0, 0.0, 0.0 });
        stbi_write_png((out_path.string() + "_px.png").c_str(), height / 2, height / 2, channels, test.data(), 0);

        test = populate_image(image, channels, height, longitude_zero_degrees, { -1.0, 1.0, -1.0 }, { 2.0, 0.0, 0.0 },
            { 0.0, 0.0, 2.0 });
        stbi_write_png((out_path.string() + "_py.png").c_str(), height / 2, height / 2, channels, test.data(), 0);

        test = populate_image(image, channels, height, longitude_zero_degrees, { 1.0, 1.0, -1.0 }, { 0.0, -2.0, 0.0 },
            { 0.0, 0.0, 2.0 });
        stbi_write_png((out_path.string() + "_pz.png").c_str(), height / 2, height / 2, channels, test.data(), 0);

        test = populate_image(image, channels, height, longitude_zero_degrees, { -1.0, 1.0, -1.0 }, { 0.0, -2.0, 0.0 },
            { 2.0, 0.0, 0.0 });
        stbi_write_png((out_path.string() + "_nx.png").c_str(), height / 2, height / 2, channels, test.data(), 0);

        test = populate_image(image, channels, height, longitude_zero_degrees, { 1.0, -1.0, -1.0 }, { -2.0, 0.0, 0.0 },
            { 0.0, 0.0, 2.0 });
        stbi_write_png((out_path.string() + "_ny.png").c_str(), height / 2, height / 2, channels, test.data(), 0);

        test = populate_image(image, channels, height, longitude_zero_degrees, { -1.0, 1.0, 1.0 }, { 0.0, -2.0, 0.0 },
            { 0.0, 0.0, -2.0 });
        stbi_write_png((out_path.string() + "_nz.png").c_str(), height / 2, height / 2, channels, test.data(), 0);
    }
    return 0;
}
