#pragma once

#include <optional>
#include "file_paths.hpp"
#include <future>

namespace vgl::file
{
    bool is_file(const std::filesystem::path& path);
    bool is_file(const char* path);
    std::optional<std::filesystem::path> save_file_dialog(const std::filesystem::path& default_path, const std::string& filter = "");
    std::optional<std::filesystem::path> open_file_dialog(const std::filesystem::path& default_path, const std::string& filter = "");
    std::optional<std::vector<std::filesystem::path>> open_multiple_files_dialog(const std::filesystem::path& default_path,
        const std::string& filter = "");
    std::string load_string_file(const std::filesystem::path& file_path);
    std::future<std::string> load_string_file_async(std::filesystem::path file_path);
    std::vector<std::byte> load_binary_file(const std::filesystem::path& file_path);
    std::future<std::vector<std::byte>> load_binary_file_async(std::filesystem::path file_path);
}
