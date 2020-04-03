#pragma once

#include <type_traits>

namespace vgl
{
    template <typename T, typename = void>
    struct is_container : std::false_type {
    };

    template <typename T>
    struct is_container<T, std::void_t<decltype(std::begin(std::declval<T>())), decltype(std::end(std::declval<T>()))>>
        : std::true_type {
    };

    template <typename T>
    constexpr bool is_container_v = is_container<T>::value;
}
