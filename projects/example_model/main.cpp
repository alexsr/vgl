#include "vgl/control/window.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "vgl/file/file.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include <execution>
#include "glm/gtc/matrix_transform.hpp"
#include <array>
#include <iostream>
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/gpu_api/gl/buffer.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include <chrono>

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

glm::vec2 to_glm_vec2(const aiVector3D& v) {
    return glm::vec2(v.x, v.y);
}

glm::vec3 to_glm_vec3(const aiVector3D& v) {
    return glm::vec3(v.x, v.y, v.z);
}

glm::vec4 to_glm_vec4(const aiVector3D& v, float w = 1.0f) {
    return glm::vec4(v.x, v.y, v.z, w);
}

std::vector<Mesh> load_mesh(const std::filesystem::path& file_path) {
    Assimp::Importer importer;
    auto scene = importer.ReadFile(file_path.string(), aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality
        | aiProcess_FindDegenerates | aiProcess_FindInvalidData | aiProcess_Triangulate | aiProcess_GenUVCoords);
    std::vector<Mesh> meshes(scene->mNumMeshes);
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        auto ai_mesh = scene->mMeshes[m];
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
        meshes[m] = mesh;
    }
    return meshes;
}

int main() {
    glfwInit();
    auto w_res = glm::ivec2(1600, 900);
    auto window = glfwCreateWindow(w_res.x, w_res.y, "Hello", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glViewport(0, 0, w_res.x, w_res.y);

    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    struct Cam_mats {
        glm::mat4 model{};
        glm::mat4 view{};
        glm::mat4 proj{};
    } cam_mats;
    cam_mats.proj = glm::perspective(glm::radians(60.0f), 16.0f/9.0f, 0.001f, 100.0f);

    struct Camera {
        glm::vec3 position{0.0f, 0.0f, -1.0f};
        glm::quat rotation{1, 0, 0, 0};
        glm::vec2 _angles{};
        void move(glm::vec3 dir, float dt) {
            position += glm::conjugate(rotation) * dir * dt;
        }
        void rotate(glm::vec2 angles , float dt) {
            _angles += angles * dt;
            rotation = glm::quat(glm::vec3(_angles.y, 0, 0)) * glm::quat(glm::vec3(0, _angles.x, 0));
        }
        void reset() {
            position = glm::vec3 {0.0f, 0.0f, -1.0f};
            rotation = glm::quat{1, 0, 0, 0};
            _angles = glm::vec3(0.0f);
        }
    } cam;

    vgl::win::window_handler win_handler(window);
    win_handler.cbs.window_size["resize"] = [](GLFWwindow*, int x, int y) {
        glViewport(0, 0, x, y);
    };
    win_handler.cbs.mouse["disable"] = [](GLFWwindow * w, int k, int a, int m) {
        if (!ImGui::IsAnyItemHovered()) {
            if (k == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_PRESS) {
                glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            if (k == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_RELEASE) {
                glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    };

    GLuint cam_ssbo = vgl::gl::create_buffer(cam_mats, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cam_ssbo);

    auto vertex_shader = vgl::gl::create_shader_spirv(GL_VERTEX_SHADER, vgl::shaders_path / "minimal/minimal.vert");
    auto frag_shader = vgl::gl::create_shader_spirv(GL_FRAGMENT_SHADER, vgl::shaders_path / "minimal/minimal.frag");

    const auto program = vgl::gl::create_program({vertex_shader, frag_shader});

    vgl::gl::delete_shaders({vertex_shader, frag_shader});
    Mesh mesh{};
    bool mesh_loaded = false;

    GLuint model_vbo = 0;
    GLuint indices_buffer = 0;
    GLuint draw_indirect_buffer = 0;
    GLuint model_vao = 0;

    glUseProgram(program);

    auto screen_vao = vgl::gl::create_vertex_array();
    using high_res_clock = std::chrono::high_resolution_clock;
    auto current = high_res_clock::now();
    auto previous = high_res_clock::now();
    float dt = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        dt = std::chrono::duration<float>(current - previous).count();
        previous = current;
        current = high_res_clock::now();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
		if (ImGui::Begin("Import Mesh")) {
			if (ImGui::Button("Load Mesh")) {
				auto file = vgl::open_file_dialog(vgl::resources_path);
				if (file) {
					mesh = load_mesh(file.value())[0];
					mesh_loaded = true;
				    model_vbo = vgl::gl::create_buffer(mesh.vertices);
                    indices_buffer = vgl::gl::create_buffer(mesh.indices);
                    Indirect_elements_command a{mesh.indices.size(), 1, 0, 0, 0};
                    draw_indirect_buffer = vgl::gl::create_buffer(a);
					model_vao = vgl::gl::create_vertex_array();
					glVertexArrayVertexBuffer(model_vao, 0, model_vbo, 0, vgl::sizeof_value_type(mesh.vertices));
                    glVertexArrayElementBuffer(model_vao, indices_buffer);
                    glEnableVertexArrayAttrib(model_vao, 0);
                    glEnableVertexArrayAttrib(model_vao, 1);
                    glEnableVertexArrayAttrib(model_vao, 2);
                    glVertexArrayAttribFormat(model_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
                    glVertexArrayAttribFormat(model_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
                    glVertexArrayAttribFormat(model_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
                    glVertexArrayAttribBinding(model_vao, 0, 0);
                    glVertexArrayAttribBinding(model_vao, 1, 0);
                    glVertexArrayAttribBinding(model_vao, 2, 0);
				}
			}
		}
        if (mesh_loaded) {
            glm::vec3 dir(static_cast<float>(win_handler.key[GLFW_KEY_A]) - static_cast<float>(win_handler.key[GLFW_KEY_D]),
                static_cast<float>(win_handler.key[GLFW_KEY_E]) - static_cast<float>(win_handler.key[GLFW_KEY_Q]),
                static_cast<float>(win_handler.key[GLFW_KEY_W]) - static_cast<float>(win_handler.key[GLFW_KEY_S]));
            if (dot(dir, dir) != 0.0f) {
                cam.move(glm::normalize(dir), dt);
            }
            if (win_handler.mouse[GLFW_MOUSE_BUTTON_LEFT]) {
                cam.rotate(glm::radians(glm::vec2(win_handler.cursor_delta)), dt);
            }
            if (win_handler.key[GLFW_KEY_R]) {
                cam.reset();
            }
            cam_mats.view = glm::mat4_cast(cam.rotation);
            cam_mats.model = glm::translate(glm::mat4(1.0f), cam.position);
            const auto buffer_ptr = glMapNamedBufferRange(cam_ssbo, 0, sizeof(cam_mats), GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT);
            std::memcpy(buffer_ptr, &cam_mats, sizeof(cam_mats));
            glUnmapNamedBuffer(cam_ssbo);
            glBindVertexArray(model_vao);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_indirect_buffer);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 1, 0);
        }
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        win_handler.poll_events();
    }

    glfwTerminate();
}
