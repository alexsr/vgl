#include "nfd.h"
#include <filesystem>
#include <optional>
#include "file_paths.hpp"

namespace vgl {
	std::optional<std::filesystem::path> open_file_dialog(std::filesystem::path default_path, const std::string& filter = "") {
		nfdchar_t* res = nullptr;
		auto status = NFD_OpenDialog(filter.c_str(), default_path.make_preferred().string().c_str(), &res);
		if (status == NFD_CANCEL || status == NFD_ERROR) {
			return std::nullopt;
		}
		return res;
	}
}
