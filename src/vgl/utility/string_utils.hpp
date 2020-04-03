#pragma once

#include <vector>
#include <string>

namespace util {
    inline std::vector<std::string> explode(std::string const& str, char const* delim) {
        std::vector<std::string> res;
        size_t start = 0;
        size_t end = 0;
        while ((end = str.find(delim, start)) != std::string::npos) {
            res.emplace_back(str.begin() + start, str.begin() + end);
            start = end + 1;
        }
        res.emplace_back(str.begin() + start, str.end());
        return res;
    }
} // namespace util
