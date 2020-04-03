#pragma once

#include <Eigen/Core>
#include "math_utils.hpp"
#undef far
#undef near

namespace vgl {
template <typename T>
Eigen::Matrix<T, 4, 4> orthographic_projection(double const left, double const right, double const top,
                                             double const bottom, double const near, double const far) {
    Eigen::Matrix<T, 4, 4> proj_mat = Eigen::Matrix<T, 4, 4>::Identity();
    auto const width = right - left;
    auto const height = top - bottom;
    auto const depth = far - near;
    proj_mat(0, 0) = static_cast<T>(2.0 / width);
    proj_mat(1, 1) = static_cast<T>(2.0 / height);
    proj_mat(2, 2) = static_cast<T>(-2.0 / depth);
    proj_mat(0, 3) = static_cast<T>(-(right + left) / width);
    proj_mat(1, 3) = static_cast<T>(-(top + bottom) / height);
    proj_mat(2, 3) = static_cast<T>(-(far + near) / depth);
    return proj_mat;
}

template <typename T>
Eigen::Matrix<T, 4, 4> orthographic_projection(double const width, double const height, double const near,
                                             double const far) {
    Eigen::Matrix<T, 4, 4> proj_mat = Eigen::Matrix<T, 4, 4>::Identity();
    proj_mat(0, 0) = static_cast<T>(2.0 / width);
    proj_mat(1, 1) = static_cast<T>(2.0 / height);
    auto const depth = far - near;
    proj_mat(2, 2) = static_cast<T>(-2.0 / depth);
    proj_mat(2, 3) = static_cast<T>(-(far + near) / depth);
    return proj_mat;
}

template <typename T>
Eigen::Matrix<T, 4, 4> frustum_projection(double const left, double const right, double const top, double const bottom,
                                        double const near, double const far) {
    Eigen::Matrix<T, 4, 4> proj_mat = Eigen::Matrix<T, 4, 4>::Identity();
    auto const width = right - left;
    auto const height = top - bottom;
    auto const depth = far - near;
    proj_mat(0, 0) = static_cast<T>(2.0 * near / width);
    proj_mat(1, 1) = static_cast<T>(2.0 * near / height);
    proj_mat(0, 2) = static_cast<T>((right + left) / width);
    proj_mat(1, 2) = static_cast<T>((top + bottom) / height);
    proj_mat(2, 2) = static_cast<T>(-(far + near) / depth);
    proj_mat(2, 3) = static_cast<T>(-2.0 * far * near / depth);
    proj_mat(3, 2) = static_cast<T>(-1.0);
    return proj_mat;
}

template <typename T>
Eigen::Matrix<T, 4, 4> perspective_projection(double const aspect_ratio, double const fovy, double const near,
                                            double const far) {
    Eigen::Matrix<T, 4, 4> proj_mat = Eigen::Matrix<T, 4, 4>::Identity();
    auto const tan_half_fovy = math::to_radians(fovy / 2.0);
    auto const depth = far - near;
    proj_mat(0, 0) = static_cast<T>(1.0 / (aspect_ratio * tan_half_fovy));
    proj_mat(1, 1) = static_cast<T>(1.0 / tan_half_fovy);
    proj_mat(2, 2) = static_cast<T>(-(far + near) / depth);
    proj_mat(2, 3) = static_cast<T>(-2.0 * far * near / depth);
    proj_mat(3, 2) = static_cast<T>(-1.0);
    return proj_mat;
}

template <typename T>
void change_aspect_ratio(Eigen::Matrix<T, 4, 4>& proj, double aspect_ratio) {
    proj(0, 0) = static_cast<T>(proj(1, 1) / aspect_ratio);
}

template <typename T>
void change_fovy(Eigen::Matrix<T, 4, 4>& proj, double fovy) {
    auto old = proj(1, 1);
    auto tan_half = std::tan(fovy / 2.0);
    proj(0, 0) = static_cast<T>(proj(0, 0) / old / tan_half);
    proj(1, 1) = static_cast<T>(1.0 / tan_half);
}
} // namespace vgl
