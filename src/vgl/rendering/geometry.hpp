#pragma once

#include <vector>
#include <array>
#include <Eigen/Core>
#include <Eigen/Geometry>

namespace vgl {
struct Vertex {
    Eigen::Vector4f pos{};
    Eigen::Vector4f normal{};
    Eigen::Vector4f tangent{};
    Eigen::Vector4f uv{};
};

inline bool operator==(const Vertex& v, const Eigen::Vector4f const& p) { return v.pos == p; }

struct Bounds;
Bounds calc_bounds(const std::vector<Vertex>& vertices);

struct Bounds {
    Eigen::Vector4f min = Eigen::Vector4f::Constant(std::numeric_limits<float>::max());
    Eigen::Vector4f max = Eigen::Vector4f::Constant(std::numeric_limits<float>::lowest());

    inline void join(const Bounds& b) {
        min = min.cwiseMin(b.min);
        max = max.cwiseMax(b.max);
    }

    inline void join(const std::vector<Vertex>& vertices) {
        Bounds b = calc_bounds(vertices);
        min = min.cwiseMin(b.min);
        max = max.cwiseMax(b.max);
    }
};

inline Bounds calc_bounds(const std::vector<Vertex>& vertices) {
    Bounds bounds{};
    for (auto& v : vertices) {
        bounds.min = bounds.min.cwiseMin(v.pos);
        bounds.max = bounds.max.cwiseMax(v.pos);
    }
    return bounds;
}
namespace geo {
inline std::array<Eigen::Vector3f, 14> unit_cube() {
    return {Eigen::Vector3f(-0.5, 0.5, 0.5),  Eigen::Vector3f(0.5, 0.5, 0.5),    Eigen::Vector3f(-0.5, -0.5, 0.5),
            Eigen::Vector3f(0.5, -0.5, 0.5),  Eigen::Vector3f(0.5, -0.5, -0.5),  Eigen::Vector3f(0.5, 0.5, 0.5),
            Eigen::Vector3f(0.5, 0.5, -0.5),  Eigen::Vector3f(-0.5, 0.5, 0.5),   Eigen::Vector3f(-0.5, 0.5, -0.5),
            Eigen::Vector3f(-0.5, -0.5, 0.5), Eigen::Vector3f(-0.5, -0.5, -0.5), Eigen::Vector3f(0.5, -0.5, -0.5),
            Eigen::Vector3f(-0.5, 0.5, -0.5), Eigen::Vector3f(0.5, 0.5, -0.5)};
}
} // namespace geo
} // namespace vgl
