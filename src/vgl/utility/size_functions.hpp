#pragma once

namespace vgl
{
    template <typename T>
    size_t sizeof_value_type(const T&) {
        return sizeof(T::value_type);
    }
}
