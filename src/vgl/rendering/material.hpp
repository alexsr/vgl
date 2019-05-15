#pragma once

#include "assimp/material.h"
#include "glm/vec4.hpp"

namespace vgl {
    struct Material {
        glm::vec4 ambient{ 1.0f };
        glm::vec4 diffuse{ 1.0f };
        glm::vec4 specular{ 0.0f };
        glm::vec4 emissive{ 0.0f };
        glm::vec4 reflective{ 0.0f };
        glm::vec4 transparent{ 0.0f };
    };

    Material process_material(aiMaterial* ai_mat) {
        Material mat{};
        ai_mat->Get(AI_MATKEY_COLOR_AMBIENT, reinterpret_cast<aiColor4D&>(mat.ambient));
        ai_mat->Get(AI_MATKEY_COLOR_DIFFUSE, reinterpret_cast<aiColor4D&>(mat.diffuse));
        ai_mat->Get(AI_MATKEY_COLOR_SPECULAR, reinterpret_cast<aiColor4D&>(mat.specular));
        ai_mat->Get(AI_MATKEY_COLOR_EMISSIVE, reinterpret_cast<aiColor4D&>(mat.emissive));
        ai_mat->Get(AI_MATKEY_COLOR_REFLECTIVE, reinterpret_cast<aiColor4D&>(mat.reflective));
        ai_mat->Get(AI_MATKEY_COLOR_TRANSPARENT, reinterpret_cast<aiColor4D&>(mat.transparent));
        ai_mat->Get(AI_MATKEY_SHININESS, mat.specular.a);
        return mat;
    }
}