#pragma once

#include <Eigen/Core>
#include <assimp/material.h>

namespace vgl {
struct Material {
    Eigen::Vector4f ambient = Eigen::Vector4f::Ones();
    Eigen::Vector4f diffuse = Eigen::Vector4f::Ones();
    Eigen::Vector4f specular = Eigen::Vector4f::Zero();
    Eigen::Vector4f emissive = Eigen::Vector4f::Zero();
    Eigen::Vector4f reflective = Eigen::Vector4f::Zero();
    Eigen::Vector4f transparent = Eigen::Vector4f::Zero();
};

inline Material process_material(aiMaterial* ai_mat) {
    Material mat{};
    ai_mat->Get(AI_MATKEY_COLOR_AMBIENT, reinterpret_cast<aiColor4D&>(mat.ambient));
    ai_mat->Get(AI_MATKEY_COLOR_DIFFUSE, reinterpret_cast<aiColor4D&>(mat.diffuse));
    ai_mat->Get(AI_MATKEY_COLOR_SPECULAR, reinterpret_cast<aiColor4D&>(mat.specular));
    ai_mat->Get(AI_MATKEY_COLOR_EMISSIVE, reinterpret_cast<aiColor4D&>(mat.emissive));
    ai_mat->Get(AI_MATKEY_COLOR_REFLECTIVE, reinterpret_cast<aiColor4D&>(mat.reflective));
    ai_mat->Get(AI_MATKEY_COLOR_TRANSPARENT, reinterpret_cast<aiColor4D&>(mat.transparent));
    ai_mat->Get(AI_MATKEY_SHININESS, mat.specular.w());
    return mat;
}
} // namespace vgl