#pragma once

#include "glm/glm.hpp"

namespace vgl {
    struct Attenuation {
        float constant = 1.0;
        float linear = 0.0;
        float quadratic = 0.0;
        float pad1 = 0.0;
    };

    struct Light {
        glm::vec4 pos{};
        glm::vec4 color{};
        glm::vec4 dir{ 0.0, -1.0, 0.0, 0.0 };
        Attenuation attenuation{};
        float outer_cutoff{ glm::pi<float>() / 4.0f };
        float inner_cutoff{ glm::pi<float>() / 4.5f };
        int type = 1;
        int pad1;
    };
}
