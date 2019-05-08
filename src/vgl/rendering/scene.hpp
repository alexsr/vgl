#pragma once

#include "glm/glm.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/material.h"
#include "assimp/postprocess.h"
#include <vector>
#include <map>
#include "vgl/file/file.hpp"
#include <filesystem>
#include <set>
#include <glm/gtx/component_wise.hpp>
#include "bounds.hpp"

namespace vgl
{
    struct Indirect_elements_command {
        unsigned int count;
        unsigned int instance_count;
        unsigned int first_index;
        unsigned int base_vertex;
        unsigned int base_instance;
    };

    struct Vertex {
        glm::vec3 pos{};
        glm::vec3 normal{};
        glm::vec2 uv{};
    };

    Bounds compute_AABB(const std::vector<Vertex>& vertices) {
        Bounds bounds{};
        for (auto& v : vertices) {
            auto p = glm::vec4(v.pos, 1.0f);
            bounds.min = glm::min(p, bounds.min);
            bounds.max = glm::max(p, bounds.max);
        }
        return bounds;
    }

    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
    };

    struct Material {
        glm::vec4 ambient{ 1.0f };
        glm::vec4 diffuse{ 1.0f };
        glm::vec4 specular{ 0.0f };
        glm::vec4 emissive{ 0.0f };
        glm::vec4 reflective{ 0.0f };
        glm::vec4 transparent{ 0.0f };
    };

    struct Texture_info {
        std::filesystem::path file_path;
        int channels{};
    };

    struct Object_range {
        unsigned int vertex_count{};
        unsigned int first_vertex{};
        unsigned int index_count{};
        unsigned int first_index{};
    };

    struct Scene_object {
        glm::mat4 model{1.0};
        int material_id = -1;
        int texture_diffuse = -1;
        int texture_specular = -1;
        int texture_normal = -1;
        int texture_height = -1;
        int texture_emissive = -1;
        int pad1;
        int pad2;
    };

    inline bool operator==(const Vertex& v, const glm::vec3& p) {
        return v.pos == p;
    }

    struct Scene {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Object_range> object_ranges;
        std::vector<Scene_object> objects;
        std::vector<Material> materials;
        std::vector<Texture_info> textures;
        Bounds scene_bounds{};
        std::vector<Bounds> object_bounds;

        void add_mesh(Mesh& m) {
            object_ranges.push_back(Object_range{
                static_cast<unsigned int>(m.vertices.size()),
                static_cast<unsigned int>(vertices.size()),
                static_cast<unsigned int>(m.indices.size()),
                static_cast<unsigned int>(indices.size())
                });
            std::move(m.indices.begin(), m.indices.end(), std::back_inserter(indices));
            std::move(m.vertices.begin(), m.vertices.end(), std::back_inserter(vertices));
            m.vertices.clear();
            m.indices.clear();
        }

        Indirect_elements_command generate_indirect_elements_cmd(unsigned int id) {
            if (id < object_ranges.size()) {
                const auto& obj = object_ranges.at(id);
                return Indirect_elements_command{ obj.index_count, 1, obj.first_index, obj.first_vertex, 0 };
            }
            return Indirect_elements_command{};
        }

        std::vector<Indirect_elements_command> generate_indirect_elements_cmds(unsigned int size = 0, unsigned int offset = 0) {
            unsigned int n = std::min(size + offset, static_cast<unsigned int>(object_ranges.size()));
            if (size == 0) {
                n = static_cast<unsigned int>(object_ranges.size());
            }
            std::vector<Indirect_elements_command> cmds;
            for (auto i = offset; i < n; i++) {
                const auto& obj = object_ranges.at(i);
                cmds.push_back(Indirect_elements_command{ obj.index_count, 1, obj.first_index, obj.first_vertex, 0 });
            }
            return cmds;
        }

        std::vector<Indirect_elements_command> generate_indirect_elements_cmds(std::initializer_list<unsigned int> ids) {
            std::vector<Indirect_elements_command> cmds;
            for (auto i : ids) {
                if (i < object_ranges.size()) {
                    const auto& obj = object_ranges.at(i);
                    cmds.push_back(Indirect_elements_command{ obj.index_count, 1, obj.first_index, obj.first_vertex, 0 });
                }
                else {
                    cmds.push_back(Indirect_elements_command{ 0, 1, 0, 0, 0 });
                }
            }
            return cmds;
        }
    };

    glm::vec2 to_glm_vec2(const aiVector3D& v) {
        return glm::vec2(v.x, v.y);
    }

    glm::vec3 to_glm_vec3(const aiVector3D& v) {
        return glm::vec3(v.x, v.y, v.z);
    }

    glm::vec4 to_glm_vec4(const aiVector3D& v, float w = 1.0f) {
        return glm::vec4(v.x, v.y, v.z, w);
    }

    Mesh generate_mesh(aiMesh* ai_mesh) {
        Mesh mesh{};
        mesh.vertices.resize(ai_mesh->mNumVertices);
        mesh.indices.resize(ai_mesh->mNumFaces * 3);

#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(ai_mesh->mNumFaces); i++) {
            if (ai_mesh->mFaces->mNumIndices != 3) {
                continue;
            }
            mesh.indices[i * 3] = ai_mesh->mFaces[i].mIndices[0];
            mesh.indices[i * 3 + 1] = ai_mesh->mFaces[i].mIndices[1];
            mesh.indices[i * 3 + 2] = ai_mesh->mFaces[i].mIndices[2];
        }
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(ai_mesh->mNumVertices); i++) {
            mesh.vertices[i].pos = to_glm_vec3(ai_mesh->mVertices[i]);
            if (ai_mesh->HasNormals()) {
                mesh.vertices[i].normal = to_glm_vec3(ai_mesh->mNormals[i]);
            }
            if (ai_mesh->HasTextureCoords(0)) {
                mesh.vertices[i].uv = to_glm_vec2(ai_mesh->mTextureCoords[0][i]);
            }
        }
        return mesh;
    }

    glm::vec4 to_glm_vec4(const aiColor3D& c) {
        return glm::vec4(c.r, c.g, c.b, 1.0f);
    }

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

    constexpr unsigned int aiprocess_flags = aiProcess_ImproveCacheLocality
        | aiProcess_FindDegenerates | aiProcess_FindInvalidData | aiProcess_RemoveRedundantMaterials |
        aiProcess_GenSmoothNormals | aiProcess_Triangulate | aiProcess_GenUVCoords | aiProcess_SortByPType;

    Mesh load_mesh(const std::filesystem::path& file_path, const unsigned int id) {
        Assimp::Importer importer;
        const auto scene = importer.ReadFile(file_path.string(),
            aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality
            | aiProcess_FindDegenerates | aiProcess_FindInvalidData |
            aiProcess_Triangulate | aiProcess_GenUVCoords);
        if (id < scene->mNumMeshes) {
            return generate_mesh(scene->mMeshes[id]);
        }
        return {};
    }

    std::vector<Mesh> load_separate_meshes(const std::filesystem::path& file_path) {
        Assimp::Importer importer;
        const auto scene = importer.ReadFile(file_path.string(),
            aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality
            | aiProcess_FindDegenerates | aiProcess_FindInvalidData |
            aiProcess_Triangulate | aiProcess_GenUVCoords);
        std::vector<Mesh> meshes(scene->mNumMeshes);
        for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
            meshes[m] = generate_mesh(scene->mMeshes[m]);
        }
        return meshes;
    }

    Scene load_scene(const std::filesystem::path& file_path) {
        Assimp::Importer importer;
        const auto ai_scene = importer.ReadFile(file_path.string(),
            aiprocess_flags);
        Scene scene{};
        auto path_parent = file_path.parent_path();
        scene.materials.resize(ai_scene->mNumMaterials);
        std::map<unsigned int, std::string> names;
#pragma omp parallel for
        for (unsigned int m = 0; m < ai_scene->mNumMaterials; ++m) {
            auto ai_mat = ai_scene->mMaterials[m];
            scene.materials.at(m) = process_material(ai_mat);
            aiString ai_name;
            auto r = ai_mat->Get(AI_MATKEY_NAME, ai_name);
            if (r != AI_SUCCESS) {
                continue;
            }
            std::string name(ai_name.C_Str());
            names[m] = name;
        }
        std::map<std::string, size_t> texture_map;
        std::map<std::string, std::string> diffuse_map;
        std::map<std::string, std::string> normal_map;
        std::map<std::string, std::string> specular_map;
        std::map<std::string, std::string> emissive_map;
        std::map<std::string, std::string> height_map;
        for (unsigned int m = 0; m < ai_scene->mNumMaterials; ++m) {
            std::string name;
            if (names.find(m) != names.end()) {
                name = names[m];
            }
            else {
                continue;
            }
            auto ai_mat = ai_scene->mMaterials[m];
            aiString path;
            auto ai_tex_available = ai_mat->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path);
            std::filesystem::path tex_path = path_parent / path.C_Str();
            if (ai_tex_available == AI_SUCCESS && is_file(file_path)) {
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
            if (ai_tex_available == AI_SUCCESS && is_file(file_path)) {
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

        scene.objects.resize(ai_scene->mNumMeshes);
        scene.object_bounds.resize(ai_scene->mNumMeshes);
        for (unsigned int m = 0; m < ai_scene->mNumMeshes; m++) {
            auto ai_mesh = ai_scene->mMeshes[m];
            auto temp = generate_mesh(ai_mesh);
            if (ai_mesh->mMaterialIndex >= 0) {
                scene.objects.at(m).material_id = static_cast<int>(ai_mesh->mMaterialIndex);
                if (names.find(ai_mesh->mMaterialIndex) != names.end()) {
                    auto name = names[ai_mesh->mMaterialIndex];
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
            scene.object_bounds.at(m) = compute_AABB(temp.vertices);
            scene.scene_bounds.combine_with(scene.object_bounds.at(m));
            scene.add_mesh(temp);
        }
        importer.FreeScene();
        return scene;
    }
}
