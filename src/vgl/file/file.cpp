#include "file.hpp"
#include "nfd.h"
#include <fstream>

bool vgl::file::is_file(const std::filesystem::path& path) {
    return exists(path) && !is_directory(path);
}

bool vgl::file::is_file(const char* path) {
    auto temp = std::filesystem::path(path);
    return exists(temp) && !is_directory(temp);
}
std::optional<std::filesystem::path> vgl::file::save_file_dialog(const std::filesystem::path& default_path,
    const std::string& filter) {
    nfdchar_t* res = nullptr;
    auto path_copy = default_path;
    auto status = NFD_SaveDialog(filter.c_str(), path_copy.make_preferred().string().c_str(), &res);
    if (status == NFD_CANCEL || status == NFD_ERROR) {
        return std::nullopt;
    }
    return res;
}

std::optional<std::filesystem::path> vgl::file::open_file_dialog(const std::filesystem::path& default_path,
    const std::string& filter) {
    nfdchar_t* res = nullptr;
    auto path_copy = default_path;
    auto status = NFD_OpenDialog(filter.c_str(), path_copy.make_preferred().string().c_str(), &res);
    if (status == NFD_CANCEL || status == NFD_ERROR) {
        return std::nullopt;
    }
    return res;
}

std::optional<std::vector<std::filesystem::path>> vgl::file::open_multiple_files_dialog(const std::filesystem::path& default_path,
    const std::string& filter) {
    nfdpathset_t res;
    auto path_copy = default_path;
    auto status = NFD_OpenDialogMultiple(filter.c_str(), path_copy.make_preferred().string().c_str(), &res);
    if (status == NFD_CANCEL || status == NFD_ERROR) {
        return std::nullopt;
    }
    std::vector<std::filesystem::path> paths(res.count);
    for (int i = 0; i < res.count; ++i) {
        paths.at(i) = res.buf + res.indices[i];
    }
    return paths;
}

std::string vgl::file::load_string_file(const std::filesystem::path& file_path) {
    std::ifstream f(file_path, std::ios::binary | std::ios::in);
    const auto file_size = std::filesystem::file_size(file_path);
    std::string buffer(file_size, ' ');
    f.read(reinterpret_cast<char*>(buffer.data()), file_size);
    return buffer;
}

std::future<std::string> vgl::file::load_string_file_async(std::filesystem::path file_path) {
    return std::async(std::launch::async, [f = std::move(file_path)]() { return load_string_file(f); });
}

std::vector<std::byte> vgl::file::load_binary_file(const std::filesystem::path& file_path) {
    std::ifstream f(file_path, std::ios::binary | std::ios::in);
    const auto file_size = std::filesystem::file_size(file_path);
    std::vector<std::byte> buffer(file_size);
    f.read(reinterpret_cast<char*>(buffer.data()), file_size);
    return buffer;
}

std::future<std::vector<std::byte>> vgl::file::load_binary_file_async(std::filesystem::path file_path) {
    return std::async(std::launch::async, [f = std::move(file_path)]() { return load_binary_file(f); });
}
