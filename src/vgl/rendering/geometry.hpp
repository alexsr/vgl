#pragma once

#include "glm/glm.hpp"
#include <vector>

namespace vgl
{
    struct Vertex {
        glm::vec3 pos{};
        glm::vec3 normal{};
        glm::vec2 uv{};
    };

    struct Bounds;
    Bounds calc_bounds(const std::vector<Vertex>& vertices);

    struct Bounds {
        glm::vec3 min{std::numeric_limits<float>::max()};
        glm::vec3 max{std::numeric_limits<float>::lowest()};

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

    Bounds calc_bounds(const std::vector<Vertex>& vertices) {
        Bounds bounds{};
        for (auto& v : vertices) {
            bounds.min = glm::min(v.pos, bounds.min);
            bounds.max = glm::max(v.pos, bounds.max);
        }
        return bounds;
    }
}
