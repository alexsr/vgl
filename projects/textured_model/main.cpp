#include "vgl/control/window.hpp"
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
#include "vgl/gpu_api/gl/texture.hpp"
#include <glsp/preprocess.hpp>
#include "vgl/file/texture_loading.hpp"
#include "vgl/gpu_api/gl/fbo.hpp"
#include "vgl/gpu_api/gl/debug.hpp"

// enable optimus!
using DWORD = uint32_t;

extern "C" {
_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

struct Attenuation {
    float constant = 1.0;
    float linear = 0.0;
    float quadratic = 0.0;
    float pad1 = 0.0;
};

struct Light {
    glm::vec4 pos{};
    glm::vec4 color{};
    glm::vec4 dir{0.0, -1.0, 0.0, 0.0};
    Attenuation attenuation{};
    float outer_cutoff{glm::pi<float>() / 4.0f};
    float inner_cutoff{ glm::pi<float>() / 4.5f };
    int type = 1;
    int pad1;
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

glm::vec4 gen_random_vec4(const float lower = 0.0f, const float upper = 1.0f, const float w = 1.0f) {
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

struct G_buffer {
    GLuint fbo = 0;
    GLuint color = 0;
    GLuint pos = 0;
    GLuint normal = 0;
    GLuint depth = 0;
    void destroy() {
        vgl::gl::delete_framebuffer(fbo);
        vgl::gl::delete_texture(color);
        vgl::gl::delete_texture(pos);
        vgl::gl::delete_texture(normal);
        vgl::gl::delete_texture(depth);
    }
};

int main() {
    auto w_res = glm::ivec2(1600, 900);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    auto window = vgl::window(w_res.x, w_res.y, "Hello");
    window.enable_gl();
    glViewport(0, 0, w_res.x, w_res.y);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(vgl::gl::debug_callback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.get(), true);
    ImGui_ImplOpenGL3_Init("#version 460");

    struct Cam_mats {
        glm::mat4 rotation{};
        glm::vec4 position{};
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
    cam_mats.rotation = glm::mat4_cast(cam.rotation);
    cam_mats.position = glm::vec4(cam.position, 1.0f);

    GLuint cam_ssbo = vgl::gl::create_buffer(cam_mats, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cam_ssbo);

    GLuint phong, depth_prepass, lights_debug, aabb_debug, screen = 0;

    auto reload_shaders = [&]() {
        auto phong_vs_future = vgl::load_string_file_async(vgl::shaders_path / "shading/phong.vert");
        auto phong_fs_future = vgl::load_string_file_async(vgl::shaders_path / "shading/phong.frag");
        auto depth_prepass_vs_future = vgl::load_string_file_async(vgl::shaders_path / "shading/depth_prepass.vert");
        auto depth_prepass_fs_future = vgl::load_string_file_async(vgl::shaders_path / "shading/depth_prepass.frag");
        auto lights_debug_vs_ftr = vgl::load_string_file_async(vgl::shaders_path / "debug/lights.vert");
        auto lights_debug_fs_ftr = vgl::load_string_file_async(vgl::shaders_path / "debug/lights.frag");
        auto screen_vs_ftr = vgl::load_string_file_async(vgl::shaders_path / "screen/screen.vert");
        auto screen_fs_ftr = vgl::load_string_file_async(vgl::shaders_path / "screen/screen.frag");
        auto aabb_debug_vs_source_ftr = vgl::load_string_file_async(vgl::shaders_path / "debug/bb.vert");
        auto aabb_debug_gs_source_ftr = vgl::load_string_file_async(vgl::shaders_path / "debug/bb.geom");
        auto aabb_debug_fs_source_ftr = vgl::load_string_file_async(vgl::shaders_path / "debug/red.frag");

        auto phong_vs_source = glsp::preprocess_source(phong_vs_future.get(), "phong.vert",
            { (vgl::shaders_path / "shading").string() }).contents;
        auto phong_fs_source = glsp::preprocess_source(phong_fs_future.get(), "phong.frag",
            { (vgl::shaders_path / "shading").string() }).contents;
        auto phong_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, phong_vs_source);
        auto phong_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, phong_fs_source);
        phong = vgl::gl::create_program({ phong_vs, phong_fs });
        vgl::gl::delete_shaders({ phong_vs, phong_fs });

        auto screen_vs_source = glsp::preprocess_source(screen_vs_ftr.get(), "screen.vert",
            { (vgl::shaders_path / "screen").string() }).contents;
        auto screen_fs_source = glsp::preprocess_source(screen_fs_ftr.get(), "screen.frag",
            { (vgl::shaders_path / "screen").string() }).contents;
        auto screen_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, screen_vs_source);
        auto screen_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, screen_fs_source);
        screen = vgl::gl::create_program({ screen_vs, screen_fs });
        vgl::gl::delete_shaders({ screen_vs, screen_fs });

        auto depth_prepass_vs_source = glsp::preprocess_source(depth_prepass_vs_future.get(), "depth_prepass.vert",
            { (vgl::shaders_path / "shading").string() }).contents;
        auto depth_prepass_fs_source = glsp::preprocess_source(depth_prepass_fs_future.get(), "depth_prepass.frag",
            { (vgl::shaders_path / "shading").string() }).contents;
        auto depth_prepass_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, depth_prepass_vs_source);
        auto depth_prepass_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, depth_prepass_fs_source);
        depth_prepass = vgl::gl::create_program({ depth_prepass_vs, depth_prepass_fs });
        vgl::gl::delete_shaders({ depth_prepass_vs, depth_prepass_fs });

        auto lights_vs_source = glsp::preprocess_source(lights_debug_vs_ftr.get(), "lights.vert",
            { (vgl::shaders_path / "debug").string() }).contents;
        auto lights_fs_source = glsp::preprocess_source(lights_debug_fs_ftr.get(), "lights.frag",
            { (vgl::shaders_path / "debug").string() }).contents;
        auto lights_debug_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, lights_vs_source);
        auto lights_debug_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, lights_fs_source);
        lights_debug = vgl::gl::create_program({ lights_debug_vs, lights_debug_fs });
        vgl::gl::delete_shaders({ lights_debug_vs, lights_debug_fs });

        auto aabb_debug_vs_source = glsp::preprocess_source(aabb_debug_vs_source_ftr.get(), "bb.vert",
            { (vgl::shaders_path / "debug").string() }).contents;
        auto aabb_debug_gs_source = glsp::preprocess_source(aabb_debug_gs_source_ftr.get(), "bb.geom",
            { (vgl::shaders_path / "debug").string() }).contents;
        auto aabb_debug_fs_source = glsp::preprocess_source(aabb_debug_fs_source_ftr.get(), "red.frag",
            { (vgl::shaders_path / "debug").string() }).contents;
        auto aabb_debug_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, aabb_debug_vs_source);
        auto aabb_debug_gs = vgl::gl::create_shader(GL_GEOMETRY_SHADER, aabb_debug_gs_source);
        auto aabb_debug_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, aabb_debug_fs_source);

        aabb_debug = vgl::gl::create_program({ aabb_debug_vs, aabb_debug_gs, aabb_debug_fs });
        vgl::gl::delete_shaders({ aabb_debug_vs, aabb_debug_gs, aabb_debug_fs });

    };
    reload_shaders();

    G_buffer g_buffer_one{};

    auto create_gbuffer = [](const glm::ivec2 & size) {
        G_buffer buffer;
        buffer.fbo = vgl::gl::create_framebuffer();
        buffer.color = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(buffer.color, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(buffer.color, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(buffer.color, 1, GL_R11F_G11F_B10F, size.x, size.y);
        glNamedFramebufferTexture(buffer.fbo, GL_COLOR_ATTACHMENT0, buffer.color, 0);
        buffer.normal = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(buffer.normal, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(buffer.normal, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(buffer.normal, 1, GL_RGB32F, size.x, size.y);
        glNamedFramebufferTexture(buffer.fbo, GL_COLOR_ATTACHMENT1, buffer.normal, 0);
        buffer.pos = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(buffer.pos, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(buffer.pos, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(buffer.pos, 1, GL_RGB32F, size.x, size.y);
        glNamedFramebufferTexture(buffer.fbo, GL_COLOR_ATTACHMENT2, buffer.pos, 0);
        buffer.depth = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(buffer.depth, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(buffer.depth, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(buffer.depth, 1, GL_DEPTH32F_STENCIL8, size.x, size.y);
        glNamedFramebufferTexture(buffer.fbo, GL_DEPTH_ATTACHMENT, buffer.depth, 0);
        vgl::gl::attach_drawbuffers(buffer.fbo, { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 });
        vgl::gl::check_framebuffer(buffer.fbo);
        return buffer;
    };

    glm::ivec2 fb_res;
    glfwGetFramebufferSize(window.get(), &fb_res.x, &fb_res.y);

    g_buffer_one = create_gbuffer(fb_res);

    window.cbs.window_size["resize"] = [&](GLFWwindow*, int x, int y) {
        glViewport(0, 0, x, y);
        w_res.x = x;
        w_res.y = y;
    };
    window.cbs.framebuffer_size["resize"] = [&](GLFWwindow*, int x, int y) {
        g_buffer_one.destroy();
        fb_res.x = x;
        fb_res.y = y;
        g_buffer_one = create_gbuffer(fb_res);
    };

    GLuint debug_vao = vgl::gl::create_vertex_array();

    vgl::Scene scene{};
    bool mesh_loaded = false;

    std::vector<Light> lights{Light{glm::vec4(0, 0, 0, 1.0), glm::vec4(1.0), glm::vec4(0.0, -1.0, 0.0, 0.0),
        Attenuation{}, glm::pi<float>() / 4.0f, 1, 0, 0} };

    GLuint model_vbo = 0;
    GLuint indices_buffer = 0;
    GLuint draw_indirect_buffer = 0;
    GLuint model_vao = 0;
    GLuint material_buffer = 0;
    GLuint mat_info_buffer = 0;
    GLuint tex_ref_buffer = 0;
    GLuint bounds_buffer = 0;

    std::vector<GLuint> textures;

    GLuint lights_ssbo = vgl::gl::create_buffer(lights, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lights_ssbo);

    auto screen_vao = vgl::gl::create_vertex_array();

    using high_res_clock = std::chrono::high_resolution_clock;
    auto current = high_res_clock::now();
    auto previous = high_res_clock::now();
    float dt = 0.0f;
    float accumulator = 0.0f;
    int frames = 0;
    float ms_p_f = 0.0f;
    while (!window.should_close()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        dt = std::chrono::duration<float>(current - previous).count();
        accumulator += dt;
        if (accumulator > 1.0f) {
            ms_p_f = accumulator / static_cast<float>(frames);
            frames = 0;
            accumulator = 0.0;
        }
        frames++;
        previous = current;
        current = high_res_clock::now();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        bool lights_added_or_removed = false;
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::MenuItem("Load Mesh")) {
                auto file = vgl::open_file_dialog(vgl::resources_path / "models");
                if (file) {
                    for (auto t : textures) {
                        vgl::gl::delete_texture(t);
                    }
                    vgl::gl::delete_buffer(model_vbo);
                    glFinish();
                    scene = vgl::load_scene(file.value());
                    textures.resize(scene.textures.size());
                    auto tex_data = vgl::load_texture_files(scene.textures);
                    std::vector<GLuint64> tex_handles(scene.textures.size());
                    for (auto i = 0; i < scene.textures.size(); ++i) {
                        auto tex_id = vgl::gl::create_texture(GL_TEXTURE_2D);
                        textures.at(i) = tex_id;
                        glTextureParameteri(tex_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTextureParameteri(tex_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTextureParameteri(tex_id, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
                        glTextureParameteri(tex_id, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
                        if (std::holds_alternative<vgl::Tex_def<stbi_uc>>(tex_data.at(i))) {
                            auto tex = std::get<vgl::Tex_def<stbi_uc>>(tex_data.at(i));
                            glTextureStorage2D(tex_id, 1, tex.internal_format, tex.image_size.x, tex.image_size.y);
                            glTextureSubImage2D(tex_id, 0, 0, 0, tex.image_size.x, tex.image_size.y, tex.format,
                                GL_UNSIGNED_BYTE, tex.ptr);
                            stbi_image_free(tex.ptr);
                        }
                        else if (std::holds_alternative<vgl::Tex_def<stbi_us>>(tex_data.at(i))) {
                            auto tex = std::get<vgl::Tex_def<stbi_us>>(tex_data.at(i));
                            glTextureStorage2D(tex_id, 1, tex.internal_format, tex.image_size.x, tex.image_size.y);
                            glTextureSubImage2D(tex_id, 0, 0, 0, tex.image_size.x, tex.image_size.y, tex.format,
                                GL_UNSIGNED_SHORT, tex.ptr);
                            stbi_image_free(tex.ptr);
                        }
                        else if (std::holds_alternative<vgl::Tex_def<float>>(tex_data.at(i))) {
                            auto tex = std::get<vgl::Tex_def<float>>(tex_data.at(i));
                            glTextureStorage2D(tex_id, 1, tex.internal_format, tex.image_size.x, tex.image_size.y);
                            glTextureSubImage2D(tex_id, 0, 0, 0, tex.image_size.x, tex.image_size.y, tex.format,
                                GL_FLOAT, tex.ptr);
                            stbi_image_free(tex.ptr);
                        }
                        tex_handles.at(i) = vgl::gl::get_texture_handle(tex_id);
                    }
                    mesh_loaded = true;
                    model_vbo = vgl::gl::create_buffer(scene.vertices);
                    indices_buffer = vgl::gl::create_buffer(scene.indices);
                    const auto draw_cmds = scene.generate_indirect_elements_cmds();
                    draw_indirect_buffer = vgl::gl::create_buffer(draw_cmds);
                    material_buffer = vgl::gl::create_buffer(scene.materials);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, material_buffer);
                    mat_info_buffer = vgl::gl::create_buffer(scene.objects);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mat_info_buffer);
                    tex_ref_buffer = vgl::gl::create_buffer(tex_handles);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, tex_ref_buffer);
                    bounds_buffer = vgl::gl::create_buffer(scene.object_bounds);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bounds_buffer);
                    model_vao = vgl::gl::create_vertex_array();
                    glVertexArrayVertexBuffer(model_vao, 0, model_vbo, 0,
                        static_cast<int>(vgl::sizeof_value_type(scene.vertices)));
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
            if (ImGui::MenuItem("Reload Shaders")) {
                reload_shaders();
            }
        }
        ImGui::EndMainMenuBar();
        if (ImGui::Begin("Settings")) {
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
                    ImGui::DragFloat3(("Attenuation##light" + std::to_string(i)).c_str(),
                        reinterpret_cast<float*>(&light_it->attenuation), 0.0001f, 0.0f, 100.0f, "%.5f");
                    if (light_it->type == 1 || light_it->type == 3) {
                        ImGui::DragFloat3(("Position##light" + std::to_string(i)).c_str(),
                            glm::value_ptr(light_it->pos), 0.01f);
                    }
                    if (light_it->type == 2 || light_it->type == 3) {
                        ImGui::DragFloat3(("Direction##light" + std::to_string(i)).c_str(),
                                            glm::value_ptr(light_it->dir), 0.01f, -1.0f, 1.0f);
                    }
                    if (light_it->type == 3) {
                        ImGui::DragFloat(("Cutoff##light" + std::to_string(i)).c_str(),
                                            &light_it->outer_cutoff, 0.01f, 0.0f, glm::pi<float>());
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
        ImGui::End();

        if (lights_added_or_removed) {
            glDeleteBuffers(1, &lights_ssbo);
            glFinish();
            lights_ssbo = vgl::gl::create_buffer(lights, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lights_ssbo);
        }
        if (mesh_loaded) {
            const auto lights_ptr = glMapNamedBufferRange(lights_ssbo, 0, sizeof(Light) * lights.size(),
                GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT |
                GL_MAP_INVALIDATE_RANGE_BIT);
            std::memcpy(lights_ptr, lights.data(), sizeof(Light) * lights.size());
            glUnmapNamedBuffer(lights_ssbo);
            if (!ImGui::IsAnyItemHovered() && !ImGui::IsAnyWindowHovered() && !ImGui::IsAnyItemFocused()
                && !ImGui::IsAnyItemActive()) {
                glm::vec3 dir(static_cast<float>(window.key[GLFW_KEY_A]) - static_cast<float>(window.key[GLFW_KEY_D]),
                    static_cast<float>(window.key[GLFW_KEY_E]) - static_cast<float>(window.key[GLFW_KEY_Q]),
                    static_cast<float>(window.key[GLFW_KEY_W]) - static_cast<float>(window.key[GLFW_KEY_S]));
                if (dot(dir, dir) != 0.0f) {
                    cam.move(glm::normalize(dir), dt);
                }
                if (window.mouse[GLFW_MOUSE_BUTTON_LEFT]) {
                    cam.rotate(glm::radians(glm::vec2(window.cursor_delta)), dt);
                }
                if (window.key[GLFW_KEY_P]) {
                    cam.reset();
                }
                if (window.key[GLFW_KEY_R]) {
                    reload_shaders();
                }
                cam_mats.rotation = glm::mat4_cast(cam.rotation);
                cam_mats.position = glm::vec4(cam.position, 1.0f);
                const auto buffer_ptr = glMapNamedBufferRange(cam_ssbo, 0, sizeof(cam_mats),
                    GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT |
                    GL_MAP_INVALIDATE_RANGE_BIT);
                std::memcpy(buffer_ptr, &cam_mats, sizeof(cam_mats));
                glUnmapNamedBuffer(cam_ssbo);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, g_buffer_one.fbo);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            glBindVertexArray(model_vao);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_indirect_buffer);
            glUseProgram(depth_prepass);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                static_cast<unsigned int>(scene.objects.size()), 0);
            glDepthFunc(GL_EQUAL);
            glUseProgram(phong);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                static_cast<unsigned int>(scene.objects.size()), 0);
            glDepthFunc(GL_LESS);
            glDisable(GL_DEPTH_TEST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindVertexArray(screen_vao);
            glProgramUniformHandleui64ARB(screen, glGetUniformLocation(screen, "tex"), vgl::gl::get_texture_handle(g_buffer_one.color));
            glUseProgram(screen);
            glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 1, 0);
            glEnable(GL_DEPTH_TEST);
            /*glBindVertexArray(debug_vao);
            glUseProgram(aabb_debug);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawArraysInstancedBaseInstance(GL_LINES, 0, 2, static_cast<int>(scene.object_bounds.size()), 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glUseProgram(lights_debug);
            glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, static_cast<int>(lights.size()), 0);*/
        }
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        window.swap_buffers();
        if (!ImGui::IsAnyItemHovered() && !ImGui::IsAnyWindowHovered() && !ImGui::IsAnyItemFocused()
            && !ImGui::IsAnyItemActive()) {
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
