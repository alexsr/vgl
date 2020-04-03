#pragma once

#include <Eigen/Core>
#include "vgl/math/constants.hpp"

namespace vgl {
    struct Attenuation {
        float constant = 1.0;
        float linear = 0.0;
        float quadratic = 0.0;
        float pad1 = 0.0;
    };

    struct Light {
        Eigen::Vector4f pos = Eigen::Vector4f::Zero();
        Eigen::Vector4f color = Eigen::Vector4f::Zero();
        Eigen::Vector4f dir = Eigen::Vector4f(0.0, -1.0, 0.0, 0.0);
        Attenuation attenuation;
        float outer_cutoff{ math::pi<float> / 4.0f };
        float inner_cutoff{ math::pi<float> / 4.5f };
        int type = 1;
        int pad1;
    };
}
