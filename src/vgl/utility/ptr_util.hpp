#pragma once

#include <memory>

namespace vgl
{
    template <typename T, typename D, typename... Args>
    std::unique_ptr<T, D> make_unique_custom_deleter(Args&& ... args) {
        return std::unique_ptr<T, D>(new T(std::forward<Args>(args)...));
    }
}
