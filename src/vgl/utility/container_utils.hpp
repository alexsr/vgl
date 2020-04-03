#pragma once

namespace vgl
{
    template<typename T>
    using element_type_t = std::remove_reference_t<decltype(*std::begin(std::declval<T&>()))>;

    template <typename T>
    size_t sizeof_value_type(const T&) {
        return sizeof(element_type_t<T>);
    }
}
