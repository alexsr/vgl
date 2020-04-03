#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <iostream>
#include "constants.hpp"

namespace vgl {
template <typename T>
void print_quat(Eigen::Quaternion<T> const& quat) {
    std::cout << quat.w() << " " << quat.x() << " " << quat.y() << " " << quat.z() << std::endl;
}

template <typename T>
Eigen::Matrix<T, 3, 1> quat_to_euler_angles(Eigen::Quaternion<T> const& q) {
    return 180.0 / math::pi<double> *
           Eigen::Matrix<T, 3, 1>(
               std::atan2(2.0 * (q.w() * q.x() + q.y() * q.z()), 1.0 - 2.0 * (q.x() * q.x() + q.y() * q.y())),
               std::asin(2.0 * (q.w() * q.y() - q.x() * q.z())),
               std::atan2(2.0 * (q.w() * q.z() + q.x() * q.y()), 1.0 - 2.0 * (q.y() * q.y() + q.z() * q.z())));
}

struct Pose {
    Pose() noexcept;
    Pose(Eigen::Quaterniond const& R, Eigen::Vector3d const& t) noexcept;
    Pose(Eigen::Matrix4d const& T) noexcept;
    Pose inverse() const;
    Eigen::Matrix4d to_matrix() const;
    void from_matrix(Eigen::Matrix4d const& T);
    void print();

    Eigen::Quaterniond rotation = Eigen::Quaterniond(1.0, 0.0, 0.0, 0.0);
    Eigen::Vector3d translation = Eigen::Vector3d(0.0, 0.0, 0.0);
};
} // namespace vgl
