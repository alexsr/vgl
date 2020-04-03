#include "pose.hpp"

vgl::Pose::Pose() noexcept {}
vgl::Pose::Pose(Eigen::Quaterniond const& R, Eigen::Vector3d const& t) noexcept
    : rotation(R.normalized()), translation(t) {}
vgl::Pose::Pose(Eigen::Matrix4d const& T) noexcept { from_matrix(T); }

vgl::Pose vgl::Pose::inverse() const { return {rotation.conjugate(), rotation.conjugate() * -translation}; }

Eigen::Matrix4d vgl::Pose::to_matrix() const {
    Eigen::Matrix4d T = Eigen::Matrix4d::Identity();
    T.block<3, 3>(0, 0) = rotation.matrix();
    T.block<3, 1>(0, 3) = translation;
    return T;
}

void vgl::Pose::from_matrix(Eigen::Matrix4d const& T) {
    rotation = Eigen::Quaterniond(T.block<3, 3>(0, 0)).normalized();
    translation = T.block<3, 1>(0, 3);
}

void vgl::Pose::print() {
    std::cout << translation.x() << " " << translation.y() << " " << translation.z() << " " << rotation.w() << " "
              << rotation.x() << " " << rotation.y() << " " << rotation.z() << std::endl;
}
