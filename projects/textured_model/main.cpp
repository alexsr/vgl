#include "vgl/control/window.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "vgl/rendering/scene.hpp"
#include "vgl/file/file.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/gpu_api/gl/buffer.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include <chrono>
#include "glm/gtc/type_ptr.hpp"
#include <random>

struct Light {
    glm::vec4 pos{};
    glm::vec4 color{};
    glm::vec4 dir{};
    float radius{};
    int type{};
    int pad1, pad2;
};

struct Material {
    glm::vec4 diffuse{};
    glm::vec4 specular{};
    glm::vec4 emissive{};
    glm::vec4 reflective{};
    glm::vec4 transparent{};
};

float gen_random_float(const float lower, const float upper) {
    std::random_device rd;
    std::mt19937 eng(rd());
    const std::uniform_real_distribution<float> uni(lower, upper);
    return uni(eng);
}

glm::vec4 gen_random_vec4(const float lower= 0.0f, const float upper = 1.0f, const float w = 1.0f) {
    return glm::vec4(gen_random_float(lower, upper), gen_random_float(lower, upper), gen_random_float(lower, upper), w);
}

std::pair<glm::vec3, glm::vec3> plane_vecs(glm::vec3 n) {
    glm::vec3 w(0, 0, 1);
    glm::vec3 n_abs(glm::abs(n));
    if (n_abs.x < glm::min(n_abs.y, n_abs.z)) {
        w = glm::vec3(1, 0, 0);
    }
    else if (n_abs.y < glm::min(n_abs.x, n_abs.z)) {
        w = glm::vec3(0, 1, 0);
    }
    glm::vec3 u = glm::normalize(cross(w, n));
    glm::vec3 v = glm::normalize(cross(n, u));
    return {u, v};
}

int main() {
    auto w_res = glm::ivec2(1600, 900);
    auto window = vgl::window(w_res.x, w_res.y, "Hello");
    window.enable_gl();
    glViewport(0, 0, w_res.x, w_res.y);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.get(), true);
    ImGui_ImplOpenGL3_Init("#version 460");

    struct Cam_mats {
        glm::mat4 view{};
        glm::mat4 proj{};
    } cam_mats;
    cam_mats.proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.001f, 100.0f);

    struct Camera {
        glm::vec3 position{0.0f, 0.0f, -1.0f};
        glm::quat rotation{1, 0, 0, 0};
        glm::vec2 _angles{};

        void move(glm::vec3 dir, float dt) {
            position += glm::conjugate(rotation) * dir * dt * 2.0f;
        }

        void rotate(glm::vec2 angles, float dt) {
            _angles += 2.0f * angles * dt;
            rotation = glm::normalize(glm::quat(glm::vec3(_angles.y, 0, 0)) * glm::quat(glm::vec3(0, _angles.x, 0)));
        }

        void reset() {
            position = glm::vec3{0.0f, 0.0f, -1.0f};
            rotation = glm::quat{1, 0, 0, 0};
            _angles = glm::vec3(0.0f);
        }
    } cam;
    cam_mats.view = glm::translate(glm::mat4_cast(cam.rotation), cam.position);

    window.cbs.window_size["resize"] = [](GLFWwindow*, int x, int y) {
        glViewport(0, 0, x, y);
    };

    GLuint cam_ssbo = vgl::gl::create_buffer(cam_mats, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cam_ssbo);

    auto vs_binary = vgl::load_binary_file_async(vgl::shaders_path / "basic_shading/normal.vert");
    auto fs_binary = vgl::load_binary_file_async(vgl::shaders_path / "basic_shading/phong.frag");
    auto vertex_shader = vgl::gl::create_shader_spirv(GL_VERTEX_SHADER, vs_binary.get());
    auto frag_shader = vgl::gl::create_shader_spirv(GL_FRAGMENT_SHADER, fs_binary.get());
    vgl::gl::specialize_shaders({ vertex_shader, frag_shader });

    const auto program = vgl::gl::create_program({vertex_shader, frag_shader});

    vgl::gl::delete_shaders({vertex_shader, frag_shader});


    auto lights_debug_vs_binary = vgl::load_binary_file_async(vgl::shaders_path / "debug/lights.vert");
    auto lights_debug_fs_binary = vgl::load_binary_file_async(vgl::shaders_path / "debug/lights.frag");
    auto lights_debug_vs = vgl::gl::create_shader_spirv(GL_VERTEX_SHADER, lights_debug_vs_binary.get());
    auto lights_debug_fs = vgl::gl::create_shader_spirv(GL_FRAGMENT_SHADER, lights_debug_fs_binary.get());
    vgl::gl::specialize_shaders({ lights_debug_vs, lights_debug_fs });

    const auto lights_debug = vgl::gl::create_program({ lights_debug_vs, lights_debug_fs });

    vgl::gl::delete_shaders({ lights_debug_vs, lights_debug_fs });

    GLuint debug_vao = vgl::gl::create_vertex_array();

    vgl::Scene mesh{};
    bool mesh_loaded = false;

    std::vector<Light> lights{Light{glm::vec4(0, 0, 0, 1.0), glm::vec4(1.0, 0, 0, 1.0), glm::vec4(0.0), 0.0f, 0, 0, 0}};

    GLuint model_vbo = 0;
    GLuint indices_buffer = 0;
    GLuint draw_indirect_buffer = 0;
    GLuint model_vao = 0;

    GLuint lights_ssbo = vgl::gl::create_buffer(lights, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lights_ssbo);

    float shininess = 0.5f;

    glProgramUniform1f(program, 0, shininess);

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
        bool lights_added_or_removed = false;
        if (ImGui::Begin("Settings")) {
            if (ImGui::Button("Load Mesh")) {
                auto file = vgl::open_file_dialog(vgl::resources_path / "models");
                if (file) {
                    mesh = vgl::load_scene(file.value());
                    mesh_loaded = true;
                    model_vbo = vgl::gl::create_buffer(mesh.vertices);
                    indices_buffer = vgl::gl::create_buffer(mesh.indices);
                    //vgl::Indirect_elements_command a{mesh.indices.size(), 1, 0, 0, 0};
                    const auto draw_cmds = mesh.generate_indirect_elements_cmds();
                    draw_indirect_buffer = vgl::gl::create_buffer(draw_cmds);
                    model_vao = vgl::gl::create_vertex_array();
                    glVertexArrayVertexBuffer(model_vao, 0, model_vbo, 0, static_cast<int>(vgl::sizeof_value_type(mesh.vertices)));
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
            ImGui::DragFloat("Shininess", &shininess, 0.01f);
            glProgramUniform1f(program, 0, shininess);
            if (ImGui::Button("Add light")) {
                lights.push_back(Light{});
                lights_added_or_removed = true;
            }
            int i = 0;
            for (auto light_it = lights.begin(); light_it != lights.end();) {
                if (ImGui::CollapsingHeader(("Light " + std::to_string(i)).c_str())) {
                    ImGui::Combo(("Type##light" + std::to_string(i)).c_str(), &light_it->type,
                                 "Ambient\0Point\0Directional\0Spotlight\0\0");
                    ImGui::Text("Type: %d", light_it->type);
                    ImGui::ColorEdit4(("Color##light" + std::to_string(i)).c_str(),
                                      glm::value_ptr(light_it->color), ImGuiColorEditFlags_Float);
                    if (light_it->type > 0) {
                        ImGui::DragFloat3(("Position##light" + std::to_string(i)).c_str(),
                                          glm::value_ptr(light_it->pos), 0.01f);
                        if (light_it->type == 2 || light_it->type == 3) {
                            ImGui::DragFloat3(("Direction##light" + std::to_string(i)).c_str(),
                                              glm::value_ptr(light_it->dir), 0.01f, 0.0f, 1.0f);
                        }
                        if (light_it->type == 3) {
                            ImGui::DragFloat(("Radius##light" + std::to_string(i)).c_str(),
                                             &light_it->radius, 0.01f, 0.0f, 100.0f);
                        }
                    }
                    if (ImGui::Button(("Delete##light" + std::to_string(i)).c_str())) {
                        light_it = lights.erase(light_it);
                        lights_added_or_removed = true;
                    }
                }
                if (lights.end() == light_it) {
                    break;
                }
                ++light_it;
                ++i;
            }
        }
        if (lights_added_or_removed) {
            glProgramUniform1i(program, 0, static_cast<int>(lights.size()));
            glDeleteBuffers(1, &lights_ssbo);
            glFinish();
            lights_ssbo = vgl::gl::create_buffer(lights, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lights_ssbo);
        }
        if (mesh_loaded) {
            const auto lights_ptr = glMapNamedBufferRange(lights_ssbo, 0, sizeof(Light) * lights.size(),
                GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
            std::memcpy(lights_ptr, lights.data(), sizeof(Light) * lights.size());
            glUnmapNamedBuffer(lights_ssbo);
            if (!ImGui::IsAnyItemHovered() && !ImGui::IsAnyWindowHovered() && !ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive()) {
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
                cam_mats.view = glm::translate(glm::mat4_cast(cam.rotation), cam.position);
                const auto buffer_ptr = glMapNamedBufferRange(cam_ssbo, 0, sizeof(cam_mats),
                    GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
                std::memcpy(buffer_ptr, &cam_mats, sizeof(cam_mats));
                glUnmapNamedBuffer(cam_ssbo);
            }
            glUseProgram(program);
            glBindVertexArray(model_vao);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_indirect_buffer);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                        static_cast<unsigned int>(mesh.objects.size()), 0);
            glUseProgram(lights_debug);
            glBindVertexArray(debug_vao);
            glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, static_cast<int>(lights.size()), 0);
        }
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        window.swap_buffers();
        if (!ImGui::IsAnyItemHovered() && !ImGui::IsAnyWindowHovered() && !ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive()) {
            if (window.mouse[GLFW_MOUSE_BUTTON_LEFT]) {
                glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else {
                glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
        window.poll_events();
    }
}
