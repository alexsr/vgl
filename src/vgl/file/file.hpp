#pragma once

#include "nfd.h"
#include <filesystem>
#include <optional>
#include "file_paths.hpp"
#include <fstream>

namespace vgl {
	std::optional<std::filesystem::path> open_file_dialog(std::filesystem::path default_path, const std::string& filter = "") {
		nfdchar_t* res = nullptr;
		auto status = NFD_OpenDialog(filter.c_str(), default_path.make_preferred().string().c_str(), &res);
		if (status == NFD_CANCEL || status == NFD_ERROR) {
			return std::nullopt;
		}
		return res;
	}

	std::vector<std::byte> load_file_binary(const std::filesystem::path& file_path) {
		std::ifstream f(file_path, std::ios::binary | std::ios::in);
		const auto file_size = std::filesystem::file_size(file_path);
		std::vector<std::byte> buffer(file_size);
		f.read(reinterpret_cast<char*>(buffer.data()), file_size);
		return buffer;
	}
}
