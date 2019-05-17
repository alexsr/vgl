#pragma once

#include "scene.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "vgl/file/file.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <execution>
#include <map>

void vgl::Scene::move_to_center() {
    auto center = glm::vec4(scene_bounds.min + scene_bounds.max) / 2.0f;
    scene_bounds.min -= center;
    scene_bounds.max -= center;
    std::for_each(std::execution::par, vertices.begin(), vertices.end(), [&center](Vertex & v) {
        v.pos -= center;
        });
    std::for_each(std::execution::par, object_bounds.begin(), object_bounds.end(), [&center](Bounds & b) {
        b.min -= center;
        b.max -= center;
        });
}

vgl::Scene vgl::load_scene(const std::filesystem::path & file_path, bool move_to_center,
    bool load_materials, bool load_textures) {
    if (!file::is_file(file_path)) {
        throw std::runtime_error{ "File not found." }; // TODO
    }
    auto path_parent = file_path.parent_path();
    Assimp::Importer importer;
    const auto ai_scene = importer.ReadFile(file_path.string(), impl::aiprocess_flags);
    if (!ai_scene) {
        throw std::runtime_error{ "File not found." }; // TODO
    }
    Scene scene{};
    scene.objects.resize(ai_scene->mNumMeshes);
    scene.object_bounds.resize(ai_scene->mNumMeshes);
    std::map<unsigned int, std::string> names;
    std::map<std::string, size_t> texture_map;
    std::map<std::string, std::string> diffuse_map;
    std::map<std::string, std::string> normal_map;
    std::map<std::string, std::string> specular_map;
    std::map<std::string, std::string> emissive_map;
    std::map<std::string, std::string> height_map;
    if (ai_scene->mNumMaterials > 0) {
        scene.materials.resize(ai_scene->mNumMaterials);
        for (unsigned int m = 0; m < ai_scene->mNumMaterials; ++m) {
            auto ai_mat = ai_scene->mMaterials[m];
            scene.materials.at(m) = process_material(ai_mat);
            aiString ai_name;
            auto r = ai_mat->Get(AI_MATKEY_NAME, ai_name);
            if (r != AI_SUCCESS) {
                continue;
            }
            names[m] = ai_name.C_Str();
            auto name = names[m];
            aiString path;
            auto ai_tex_available = ai_mat->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path);
            std::filesystem::path tex_path = path_parent / path.C_Str();
            if (ai_tex_available == AI_SUCCESS && file::is_file(file_path)) {
                diffuse_map[name] = path.C_Str();
                if (texture_map.emplace(path.C_Str(), scene.textures.size()).second) {
                    scene.textures.emplace_back(Texture_info{ tex_path, 4 });
                }
            }
            /*ai_tex_available = ai_mat->Get(AI_MATKEY_TEXTURE_NORMALS(0), path);
            tex_path = path_parent / path.C_Str();
            if (ai_tex_available == AI_SUCCESS && is_file(file_path)) {
                normal_map[name] = path.C_Str();
                if (texture_map.emplace(path.C_Str(), scene.textures.size()).second) {
                    scene.textures.emplace_back(Texture_info{ tex_path, 3 });
                }
            }*/
            ai_tex_available = ai_mat->Get(AI_MATKEY_TEXTURE_SPECULAR(0), path);
            tex_path = path_parent / path.C_Str();
            if (ai_tex_available == AI_SUCCESS && file::is_file(file_path)) {
                specular_map[name] = path.C_Str();
                if (texture_map.emplace(path.C_Str(), scene.textures.size()).second) {
                    scene.textures.emplace_back(Texture_info{ tex_path, 4 });
                }
            }
            /*ai_tex_available = ai_mat->Get(AI_MATKEY_TEXTURE_EMISSIVE(0), path);
            tex_path = path_parent / path.C_Str();
            if (ai_tex_available == AI_SUCCESS && is_file(file_path)) {
                emissive_map[name] = path.C_Str();
                if (texture_map.emplace(path.C_Str(), scene.textures.size()).second) {
                    scene.textures.emplace_back(Texture_info{ tex_path, 4 });
                }
            }
            ai_tex_available = ai_mat->Get(AI_MATKEY_TEXTURE_HEIGHT(0), path);
            tex_path = path_parent / path.C_Str();
            if (ai_tex_available == AI_SUCCESS && is_file(file_path)) {
                height_map[name] = path.C_Str();
                if (texture_map.emplace(path.C_Str(), scene.textures.size()).second) {
                    scene.textures.emplace_back(Texture_info{ tex_path, 1 });
                }
            }*/
        }
    }
    std::vector<size_t> start_indices(ai_scene->mNumMeshes + 1, 0);
    std::vector<size_t> start_vertices(ai_scene->mNumMeshes + 1, 0);
    for (unsigned int m = 0; m < ai_scene->mNumMeshes; m++) {
        auto ai_mesh = ai_scene->mMeshes[m];
        start_indices.at(m + 1) = start_indices.at(m) + ai_mesh->mNumFaces;
        start_vertices.at(m + 1) = start_vertices.at(m) + ai_mesh->mNumVertices;
    }
    scene.indices.resize(start_indices.back() * 3);
    scene.vertices.resize(start_vertices.back());
#pragma omp parallel for
    for (unsigned int m = 0; m < ai_scene->mNumMeshes; m++) {
        auto ai_mesh = ai_scene->mMeshes[m];
        scene.draw_cmds.push_back(gl::Indirect_elements_command{
            static_cast<unsigned int>(ai_mesh->mNumFaces * 3),
            1,
            static_cast<unsigned int>(start_indices.at(m) * 3),
            static_cast<unsigned int>(start_vertices.at(m)),
            0
            });
        for (int i = 0; i < static_cast<int>(ai_mesh->mNumFaces); i++) {
            auto scene_i = start_indices.at(m) + i;
            if (ai_mesh->mFaces->mNumIndices != 3) {
                continue;
            }
            scene.indices[scene_i * 3] = ai_mesh->mFaces[i].mIndices[0];
            scene.indices[scene_i * 3 + 1] = ai_mesh->mFaces[i].mIndices[1];
            scene.indices[scene_i * 3 + 2] = ai_mesh->mFaces[i].mIndices[2];
        }
        Bounds bounds{};
        for (int i = 0; i < static_cast<int>(ai_mesh->mNumVertices); i++) {
            auto scene_i = start_vertices.at(m) + i;
            scene.vertices[scene_i].pos = glm::vec4(reinterpret_cast<glm::vec3&>(ai_mesh->mVertices[i]), 1.0f);
            bounds.min = glm::min(bounds.min, scene.vertices[scene_i].pos);
            bounds.max = glm::max(bounds.max, scene.vertices[scene_i].pos);
            if (ai_mesh->HasNormals()) {
                scene.vertices[scene_i].normal = glm::vec4(reinterpret_cast<glm::vec3&>(ai_mesh->mNormals[i]), 0.0f);
            }
            if (ai_mesh->HasTextureCoords(0)) {
                scene.vertices[scene_i].uv = glm::vec4(reinterpret_cast<glm::vec2&>(ai_mesh->mTextureCoords[0][i]), 0.0f, 0.0f);
            }
        }
        scene.object_bounds.at(m) = bounds;
        scene.scene_bounds.join(bounds);
        if (ai_scene->mMeshes[m]->mMaterialIndex >= 0) {
            scene.objects.at(m).material_id = static_cast<int>(ai_scene->mMeshes[m]->mMaterialIndex);
            if (names.find(ai_scene->mMeshes[m]->mMaterialIndex) != names.end()) {
                auto name = names[ai_scene->mMeshes[m]->mMaterialIndex];
                if (diffuse_map.find(name) != diffuse_map.end()) {
                    scene.objects.at(m).texture_diffuse = static_cast<int>(texture_map[diffuse_map[name]]);
                }
                if (normal_map.find(name) != normal_map.end()) {
                    scene.objects.at(m).texture_normal = static_cast<int>(texture_map[normal_map[name]]);
                }
                if (specular_map.find(name) != specular_map.end()) {
                    scene.objects.at(m).texture_specular = static_cast<int>(texture_map[specular_map[name]]);
                }
                if (emissive_map.find(name) != emissive_map.end()) {
                    scene.objects.at(m).texture_emissive = static_cast<int>(texture_map[emissive_map[name]]);
                }
                if (height_map.find(name) != height_map.end()) {
                    scene.objects.at(m).texture_height = static_cast<int>(texture_map[height_map[name]]);
                }
            }
        }
    }
    if (move_to_center) {
        scene.move_to_center();
    }
    importer.FreeScene();
    return scene;
}
