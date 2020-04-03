#pragma once

#include <vector>
#include <filesystem>
#include "geometry.hpp"
#include "material.hpp"
#include "vgl/gpu_api/gl/multidraw.hpp"
#include "vgl/file/image_file.hpp"
#include <assimp/postprocess.h>

namespace vgl
{
    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
    };

    struct Scene_object {
        Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
        int material_id = -1;
        int texture_diffuse = -1;
        int texture_specular = -1;
        int texture_normal = -1;
        int texture_height = -1;
        int texture_emissive = -1;
        int pad1 = 0;
        int pad2 = 0;
    };

    struct Scene {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        Eigen::AlignedBox4f scene_bounds{};
        std::vector<Eigen::AlignedBox4f> object_bounds;
        std::vector<Scene_object> objects;
        std::vector<gl::Indirect_elements_command> draw_cmds;
        std::vector<Material> materials;
        std::vector<Image_info> textures;

        void move_to_center();
        void join_copy(const Scene& s);
        void join(Scene& s);
    };

    namespace impl {
        constexpr unsigned int aiprocess_flags = aiProcess_ImproveCacheLocality | aiProcess_JoinIdenticalVertices
            | aiProcess_CalcTangentSpace | aiProcess_FindDegenerates | aiProcess_FindInvalidData
            | aiProcess_RemoveRedundantMaterials | aiProcess_GenSmoothNormals | aiProcess_Triangulate
            | aiProcess_GenUVCoords | aiProcess_SortByPType;
    }

    Scene load_scene(const std::filesystem::path& file_path, bool move_to_center = false,
        bool load_materials = true, bool load_images = true);
}
