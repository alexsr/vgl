#pragma once

#include "constants.hpp"
#include <Eigen/Core>

namespace math {
template <typename T>
T to_radians(T degree) {
    return degree / static_cast<T>(180.0) * pi<T>;
}

template <typename T, int rows, int cols>
Eigen::Matrix<T, rows, cols> to_radians(Eigen::Matrix<T, rows, cols> degree) {
    return degree / static_cast<T>(180.0) * pi<T>;
}

template <typename T>
T to_degree(T radians) {
    return radians * static_cast<T>(180.0) / pi<T>;
}

template <typename T, int rows, int cols>
Eigen::Matrix<T, rows, cols> to_degree(Eigen::Matrix<T, rows, cols> radians) {
    return radians * static_cast<T>(180.0) / pi<T>;
}
} // namespace math
