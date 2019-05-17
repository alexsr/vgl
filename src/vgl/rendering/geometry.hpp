#pragma once

#include "glm/glm.hpp"
#include <vector>
#include <array>

namespace vgl
{
    struct Vertex {
        glm::vec4 pos{};
        glm::vec4 normal{};
        glm::vec4 uv{};
    };

    inline bool operator==(const Vertex& v, const glm::vec4& p) {
        return v.pos == p;
    }

    struct Bounds;
    Bounds calc_bounds(const std::vector<Vertex>& vertices);

    struct Bounds {
        glm::vec4 min{std::numeric_limits<float>::max()};
        glm::vec4 max{std::numeric_limits<float>::lowest()};

        inline void join(const Bounds& b) {
            min = glm::min(min, b.min);
            max = glm::max(max, b.max);
        }

        inline void join(const std::vector<Vertex>& vertices) {
            Bounds b = calc_bounds(vertices);
            min = glm::min(min, b.min);
            max = glm::max(max, b.max);
        }
    };

    inline Bounds calc_bounds(const std::vector<Vertex>& vertices) {
        Bounds bounds{};
        for (auto& v : vertices) {
            bounds.min = glm::min(v.pos, bounds.min);
            bounds.max = glm::max(v.pos, bounds.max);
        }
        return bounds;
    }
    namespace geo {
        constexpr std::array<glm::vec3, 14> unit_cube{ glm::vec3(-0.5, 0.5, 0.5),
                    glm::vec3(0.5, 0.5, 0.5), glm::vec3(-0.5, -0.5, 0.5),
                    glm::vec3(0.5, -0.5, 0.5), glm::vec3(0.5, -0.5, -0.5),
                    glm::vec3(0.5, 0.5, 0.5), glm::vec3(0.5, 0.5, -0.5),
                    glm::vec3(-0.5, 0.5, 0.5), glm::vec3(-0.5, 0.5, -0.5),
                    glm::vec3(-0.5, -0.5, 0.5), glm::vec3(-0.5, -0.5, -0.5),
                    glm::vec3(0.5, -0.5, -0.5), glm::vec3(-0.5, 0.5, -0.5), glm::vec3(0.5, 0.5, -0.5) };
    }
}
