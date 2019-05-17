#pragma once

#include <filesystem>

namespace vgl {
    struct Texture_info {
        std::filesystem::path file_path;
        int channels{};
    };
}
