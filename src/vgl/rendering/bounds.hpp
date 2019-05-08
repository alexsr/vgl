#pragma once

#include "glm/glm.hpp"

namespace vgl
{
    struct Bounds {
        glm::vec4 min{FLT_MAX};
        glm::vec4 max{-FLT_MAX};

        void combine_with(const Bounds& b) {
            min = glm::min(min, b.min);
            max = glm::max(max, b.max);
        }
    };
}
