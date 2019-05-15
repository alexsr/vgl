#pragma once

#include "nfd.h"
#include <optional>
#include "file_paths.hpp"
#include <fstream>
#include <future>

namespace vgl::file
{
    bool is_file(const std::filesystem::path& path) {
        return exists(path) && !is_directory(path);
    }

    bool is_file(const char* path) {
        auto temp = std::filesystem::path(path);
        return exists(temp) && !is_directory(temp);
    }

    std::optional<std::filesystem::path> open_file_dialog(std::filesystem::path default_path,
        const std::string& filter = "") {
        nfdchar_t* res = nullptr;
        auto status = NFD_OpenDialog(filter.c_str(), default_path.make_preferred().string().c_str(), &res);
        if (status == NFD_CANCEL || status == NFD_ERROR) {
            return std::nullopt;
        }
        return res;
    }

    std::string load_string_file(const std::filesystem::path& file_path) {
        std::ifstream f(file_path, std::ios::binary | std::ios::in);
        const auto file_size = std::filesystem::file_size(file_path);
        std::string buffer(file_size, ' ');
        f.read(reinterpret_cast<char*>(buffer.data()), file_size);
        return buffer;
    }

    std::future<std::string> load_string_file_async(std::filesystem::path file_path) {
        return std::async(std::launch::async, [f = std::move(file_path)]() { return load_string_file(f); });
    }

    std::vector<std::byte> load_binary_file(const std::filesystem::path& file_path) {
        std::ifstream f(file_path, std::ios::binary | std::ios::in);
        const auto file_size = std::filesystem::file_size(file_path);
        std::vector<std::byte> buffer(file_size);
        f.read(reinterpret_cast<char*>(buffer.data()), file_size);
        return buffer;
    }

    std::future<std::vector<std::byte>> load_binary_file_async(std::filesystem::path file_path) {
        return std::async(std::launch::async, [f = std::move(file_path)]() { return load_binary_file(f); });
    }
}
