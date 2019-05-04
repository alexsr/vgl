#include "vgl/control/window.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "vgl/mesh/mesh.hpp"
#include "vgl/file/file.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/gpu_api/gl/buffer.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include <chrono>
#include <numeric>

int main() {
    auto w_res = glm::ivec2(1600, 900);
    auto window = vgl::window(w_res.x, w_res.y, "Hello");
    window.enable_gl();
    glViewport(0, 0, w_res.x, w_res.y);

    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.get(), true);
    ImGui_ImplOpenGL3_Init("#version 460");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    struct Cam_mats {
        glm::mat4 model{};
        glm::mat4 view{};
        glm::mat4 proj{};
    } cam_mats;
    cam_mats.proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.001f, 100.0f);

    struct Camera {
        glm::vec3 position{0.0f, 0.0f, -1.0f};
        glm::quat rotation{1, 0, 0, 0};
        glm::vec2 _angles{};

        void move(glm::vec3 dir, float dt) {
            position += glm::conjugate(rotation) * dir * dt;
        }

        void rotate(glm::vec2 angles, float dt) {
            _angles += angles * dt;
            rotation = glm::normalize(glm::quat(glm::vec3(_angles.y, 0, 0)) * glm::quat(glm::vec3(0, _angles.x, 0)));
        }

        void reset() {
            position = glm::vec3{0.0f, 0.0f, -1.0f};
            rotation = glm::quat{1, 0, 0, 0};
            _angles = glm::vec3(0.0f);
        }
    } cam;

    window.cbs.window_size["resize"] = [](GLFWwindow*, int x, int y) {
        glViewport(0, 0, x, y);
    };
    window.cbs.mouse["disable"] = [](GLFWwindow* w, int k, int a, int m) {
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

    vgl::Scene mesh{};
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
    while (!window.should_close()) {
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
                    mesh = vgl::load_scene(file.value());
                    mesh_loaded = true;
                    model_vbo = vgl::gl::create_buffer(mesh.vertices);
                    indices_buffer = vgl::gl::create_buffer(mesh.indices);
                    //vgl::Indirect_elements_command a{mesh.indices.size(), 1, 0, 0, 0};
                    auto a = mesh.generate_indirect_elements_cmds();
                    draw_indirect_buffer = vgl::gl::create_buffer(a);
                    model_vao = vgl::gl::create_vertex_array();
                    glVertexArrayVertexBuffer(model_vao, 0, model_vbo, 0, vgl::sizeof_value_type(mesh.vertices));
                    glVertexArrayElementBuffer(model_vao, indices_buffer);
                    glEnableVertexArrayAttrib(model_vao, 0);
                    glEnableVertexArrayAttrib(model_vao, 1);
                    glEnableVertexArrayAttrib(model_vao, 2);
                    glVertexArrayAttribFormat(model_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(vgl::Vertex, pos));
                    glVertexArrayAttribFormat(model_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(vgl::Vertex, normal));
                    glVertexArrayAttribFormat(model_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(vgl::Vertex, uv));
                    glVertexArrayAttribBinding(model_vao, 0, 0);
                    glVertexArrayAttribBinding(model_vao, 1, 0);
                    glVertexArrayAttribBinding(model_vao, 2, 0);
                }
            }
        }
        if (mesh_loaded) {
            glm::vec3 dir(static_cast<float>(window.key[GLFW_KEY_A]) - static_cast<float>(window.key[GLFW_KEY_D]),
                          static_cast<float>(window.key[GLFW_KEY_E]) - static_cast<float>(window.key[GLFW_KEY_Q]),
                          static_cast<float>(window.key[GLFW_KEY_W]) - static_cast<float>(window.key[GLFW_KEY_S]));
            if (dot(dir, dir) != 0.0f) {
                cam.move(glm::normalize(dir), dt);
            }
            if (window.mouse[GLFW_MOUSE_BUTTON_LEFT]) {
                cam.rotate(glm::radians(glm::vec2(window.cursor_delta)), dt);
            }
            if (window.key[GLFW_KEY_R]) {
                cam.reset();
            }
            cam_mats.view = glm::mat4_cast(cam.rotation);
            cam_mats.model = glm::translate(glm::mat4(1.0f), cam.position);
            const auto buffer_ptr = glMapNamedBufferRange(cam_ssbo, 0, sizeof(cam_mats),
                                                          GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT);
            std::memcpy(buffer_ptr, &cam_mats, sizeof(cam_mats));
            glUnmapNamedBuffer(cam_ssbo);
            glBindVertexArray(model_vao);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_indirect_buffer);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, static_cast<unsigned int>(mesh.objects.size()), 0);
        }
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        window.swap_buffers();
        window.poll_events();
    }
}
