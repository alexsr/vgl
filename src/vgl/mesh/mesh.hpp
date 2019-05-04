#pragma once

#include "glm/glm.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <vector>
#include "vgl/file/file.hpp"

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

    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
    };

    struct Scene_object {
        unsigned int vertex_count{};
        unsigned int first_vertex{};
        unsigned int index_count{};
        unsigned int first_index{};
        glm::mat4 transform{};
    };

    struct Scene {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Scene_object> objects;

        void add_mesh(Mesh& m) {
            objects.push_back(Scene_object{
                static_cast<unsigned int>(m.vertices.size()),
                static_cast<unsigned int>(vertices.size()),
                static_cast<unsigned int>(m.indices.size()),
                static_cast<unsigned int>(indices.size()),
                glm::mat4(1.0f)
            });
            std::move(m.indices.begin(), m.indices.end(), std::back_inserter(indices));
            std::move(m.vertices.begin(), m.vertices.end(), std::back_inserter(vertices));
            m.vertices.clear();
            m.indices.clear();
        }

        Indirect_elements_command generate_indirect_elements_cmd(unsigned int id) {
            if (id < objects.size()) {
                const auto& obj = objects.at(id);
                return Indirect_elements_command{ obj.index_count, 1, obj.first_index, obj.first_vertex, 0 };
            }
            return Indirect_elements_command{};
        }

        std::vector<Indirect_elements_command> generate_indirect_elements_cmds(unsigned int size = 0, unsigned int offset = 0) {
            unsigned int n = std::min(size + offset, static_cast<unsigned int>(objects.size()));
            if (size == 0) {
                n = static_cast<unsigned int>(objects.size());
            }
            std::vector<Indirect_elements_command> cmds;
            for (auto i = offset; i < n; i++) {
                const auto& obj = objects.at(i);
                cmds.push_back(Indirect_elements_command{ obj.index_count, 1, obj.first_index, obj.first_vertex, 0 });
            }
            return cmds;
        }

        std::vector<Indirect_elements_command> generate_indirect_elements_cmds(std::initializer_list<unsigned int> ids) {
            std::vector<Indirect_elements_command> cmds;
            for (auto i : ids) {
                if (i < objects.size()) {
                    const auto& obj = objects.at(i);
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

    std::vector<Mesh> load_separate_meshes(const std::filesystem::path& file_path) {
        Assimp::Importer importer;
        const auto scene = importer.ReadFile(file_path.string(),
                                             aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality
                                             | aiProcess_FindDegenerates | aiProcess_FindInvalidData |
                                             aiProcess_Triangulate |
                                             aiProcess_GenUVCoords);
        std::vector<Mesh> meshes(scene->mNumMeshes);
        for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
            meshes[m] = generate_mesh(scene->mMeshes[m]);
        }
        return meshes;
    }

    Scene load_scene(const std::filesystem::path& file_path) {
        Assimp::Importer importer;
        const auto ai_scene = importer.ReadFile(file_path.string(),
                                                aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality
                                                | aiProcess_FindDegenerates | aiProcess_FindInvalidData |
                                                aiProcess_Triangulate |
                                                aiProcess_GenUVCoords);
        Scene scene{};
        for (unsigned int m = 0; m < ai_scene->mNumMeshes; m++) {
            auto temp = generate_mesh(ai_scene->mMeshes[m]);
            scene.add_mesh(temp);
        }
        return scene;
    }
}
