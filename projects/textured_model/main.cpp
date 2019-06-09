#include "vgl/control/window.hpp"
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
#include "vgl/file/image_file.hpp"
#include "vgl/gpu_api/gl/framebuffer.hpp"
#include "vgl/gpu_api/gl/debug.hpp"
#include "vgl/control/gui.hpp"
#include "vgl/rendering/camera.hpp"
#include "vgl/rendering/light.hpp"

// enable optimus!
using DWORD = uint32_t;

extern "C" {
_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

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
    vgl::gl::glframebuffer fbo;
    vgl::gl::gltexture color;
    vgl::gl::gltexture specular;
    vgl::gl::gltexture pos;
    vgl::gl::gltexture normal;
    vgl::gl::gltexture depth;
    void init(glm::ivec2 size) {
        fbo = vgl::gl::create_framebuffer();
        color = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(color, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(color, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(color, 1, GL_RGBA32F, size.x, size.y);
        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, color, 0);
        specular = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(specular, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(specular, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(specular, 1, GL_RGBA32F, size.x, size.y);
        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT1, color, 0);
        normal = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(normal, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(normal, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(normal, 1, GL_RGB32F, size.x, size.y);
        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT2, normal, 0);
        pos = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(pos, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(pos, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(pos, 1, GL_RGB32F, size.x, size.y);
        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT3, pos, 0);
        depth = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(depth, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(depth, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(depth, 1, GL_DEPTH32F_STENCIL8, size.x, size.y);
        glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, depth, 0);
        vgl::gl::check_framebuffer(fbo);
    }
};

int main() {
    auto w_res = glm::ivec2(1600, 900);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    vgl::Window window(w_res.x, w_res.y, "Hello");
    window.enable_gl();
    glfwSwapInterval(0);
    glViewport(0, 0, w_res.x, w_res.y);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(vgl::gl::debug_callback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_MULTISAMPLE);
    int samples = 0;
    glGetIntegerv(GL_SAMPLES, &samples);
    std::cout << samples << "\n";
    vgl::ui::Gui gui(window);

    struct Cubemap_config {
        bool hdr = false;
        float gamma = 2.2f;
        float exposure = 1.0f;
        int tone_mapping = 0;
    };

    struct Demo_config {
        std::vector<vgl::Light> lights{ vgl::Light{glm::vec4(0, 0, 0, 1.0), glm::vec4(1.0), glm::vec4(0.0, -1.0, 0.0, 0.0),
            vgl::Attenuation{}, glm::pi<float>() / 4.0f, 1, 1, 0} };
        glm::ivec2 fb_res;
        bool mesh_loaded = false;
        bool lights_added_or_removed = false;
        bool cubemap_active = false;
        bool cubemap_equirectangular = false;
        Cubemap_config cm_conf;
        bool msaa = false;
        float scale = 1.0f;
    } config;
    glfwGetFramebufferSize(window.get(), &config.fb_res.x, &config.fb_res.y);

    vgl::Camera cam;
    cam.rotation_speed = 3.0f;
    cam.projection = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.001f, 100.0f);

    auto cam_ssbo = vgl::gl::create_buffer(cam.get_cam_data(), GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cam_ssbo);

    vgl::gl::glprogram phong, depth_prepass, lights_debug, aabb_debug, screen, cubemap, cubemap_equirect;

    auto reload_shaders = [&]() {
        auto phong_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "shading/shading.vert").contents;
        auto phong_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "shading/phong.frag").contents;
        auto phong_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, phong_vs_source);
        auto phong_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, phong_fs_source);
        phong = vgl::gl::create_program({ phong_vs, phong_fs });

        auto screen_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "util/screen.vert").contents;
        auto screen_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "util/screen.frag").contents;
        auto screen_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, screen_vs_source);
        auto screen_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, screen_fs_source);
        screen = vgl::gl::create_program({ screen_vs, screen_fs });

        auto cubemap_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "util/cubemap.vert").contents;
        auto cubemap_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "util/cubemap.frag").contents;
        auto cubemap_equirect_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "util/cubemap_equirectangular.frag").contents;
        auto cubemap_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, cubemap_vs_source);
        auto cubemap_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, cubemap_fs_source);
        auto cubemap_equirect_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, cubemap_equirect_fs_source);
        cubemap = vgl::gl::create_program({ cubemap_vs, cubemap_fs });
        cubemap_equirect = vgl::gl::create_program({ cubemap_vs, cubemap_equirect_fs });

        auto depth_prepass_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "shading/depth_prepass.vert").contents;
        auto depth_prepass_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "shading/depth_prepass.frag").contents;
        auto depth_prepass_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, depth_prepass_vs_source);
        auto depth_prepass_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, depth_prepass_fs_source);
        depth_prepass = vgl::gl::create_program({ depth_prepass_vs, depth_prepass_fs });

        auto lights_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "debug/lights.vert").contents;
        auto lights_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "debug/lights.frag").contents;
        auto lights_debug_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, lights_vs_source);
        auto lights_debug_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, lights_fs_source);
        lights_debug = vgl::gl::create_program({ lights_debug_vs, lights_debug_fs });

        auto aabb_debug_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "debug/bb.vert").contents;
        auto aabb_debug_gs_source = glsp::preprocess_file(vgl::file::shaders_path / "debug/bb.geom").contents;
        auto aabb_debug_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "debug/red.frag").contents;
        auto aabb_debug_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, aabb_debug_vs_source);
        auto aabb_debug_gs = vgl::gl::create_shader(GL_GEOMETRY_SHADER, aabb_debug_gs_source);
        auto aabb_debug_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, aabb_debug_fs_source);
        aabb_debug = vgl::gl::create_program({ aabb_debug_vs, aabb_debug_gs, aabb_debug_fs });
    };
    reload_shaders();

    G_buffer g_buffer_one{};
    g_buffer_one.init(config.fb_res);

    window.cbs.window_size["resize"] = [&](GLFWwindow*, int x, int y) {
        w_res.x = x;
        w_res.y = y;
    };
    window.cbs.framebuffer_size["resize"] = [&](GLFWwindow*, int x, int y) {
        if (x > 0 && y > 0 && (x != config.fb_res.x || y != config.fb_res.y)) {
            config.fb_res.x = x;
            config.fb_res.y = y;
            g_buffer_one.init(config.fb_res);
            glViewport(0, 0, x, y);
            cam.change_aspect_ratio(x / static_cast<float>(y));
            vgl::gl::update_full_buffer(cam_ssbo, cam.get_cam_data());
        }
    };

    auto debug_vao = vgl::gl::create_vertex_array();

    vgl::Scene scene{};

    vgl::gl::glbuffer model_vbo = 0;
    vgl::gl::glbuffer indices_buffer = 0;
    vgl::gl::glbuffer draw_indirect_buffer = 0;
    vgl::gl::glvertexarray model_vao = 0;
    vgl::gl::glbuffer material_buffer = 0;
    vgl::gl::glbuffer mat_info_buffer = 0;
    vgl::gl::glbuffer tex_ref_buffer = 0;
    vgl::gl::glbuffer bounds_buffer = 0;

    auto cube_vao = vgl::gl::create_vertex_array();
    auto cube_buffer = vgl::gl::create_buffer(vgl::geo::unit_cube);
    glVertexArrayVertexBuffer(cube_vao, 0, cube_buffer, 0, sizeof(glm::vec3));
    glEnableVertexArrayAttrib(cube_vao, 0);
    glVertexArrayAttribFormat(cube_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(cube_vao, 0, 0);

    auto cubemap_config_ssbo = vgl::gl::create_buffer(config.cm_conf, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, cubemap_config_ssbo);

    vgl::gl::gltexture cubemap_texture;

    std::vector<vgl::gl::gltexture> textures;

    auto lights_ssbo = vgl::gl::create_buffer(config.lights, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
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
        gui.start_frame();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        dt = std::chrono::duration<float>(current - previous).count();
        accumulator += dt;
        if (accumulator > 1.0f) {
            ms_p_f = accumulator / static_cast<float>(frames);
            frames = 0;
            accumulator = 0.0;
        }
        if (ImGui::Begin("Runtime")) {
            ImGui::Text("Milliseconds per frame: %f", ms_p_f);
        }
        ImGui::End();
        frames++;
        previous = current;
        current = high_res_clock::now();
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::MenuItem("Load Mesh")) {
                auto file = vgl::file::open_file_dialog(vgl::file::resources_path / "models");
                if (file) {
                    textures.clear();
                    glFinish();
                    scene = vgl::load_scene(file.value(), true);
                    textures.resize(scene.textures.size());
                    auto tex_data = vgl::file::load_images(scene.textures);
                    std::vector<GLuint64> tex_handles(scene.textures.size());
                    for (auto i = 0; i < scene.textures.size(); ++i) {
                        textures.at(i) = vgl::gl::create_texture(GL_TEXTURE_2D);
                        glTextureParameteri(textures.at(i), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTextureParameteri(textures.at(i), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTextureParameteri(textures.at(i), GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
                        glTextureParameteri(textures.at(i), GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
                        glTextureStorage2D(textures.at(i), 1, tex_data.at(i).desc.internal_format,
                            tex_data.at(i).desc.image_size.x, tex_data.at(i).desc.image_size.y);
                        vgl::gl::set_texture_data_2d(textures.at(i), tex_data.at(i));
                        tex_handles.at(i) = vgl::gl::get_texture_handle(textures.at(i));
                    }
                    config.mesh_loaded = true;
                    indices_buffer = vgl::gl::create_buffer(scene.indices);
                    model_vbo = vgl::gl::create_buffer(scene.vertices);
                    draw_indirect_buffer = vgl::gl::create_buffer(scene.draw_cmds);
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
                    cam.move_speed = sqrt(length((scene.scene_bounds.min - scene.scene_bounds.max) / 2.0f));
                }
            }
            if (ImGui::BeginMenu("Cubemap")) {
                if (ImGui::MenuItem("Load cubemap faces")) {
                    auto files = vgl::file::open_multiple_files_dialog(vgl::file::resources_path / "images");
                    if (files) {
                        if (files->size() == 6) {
                            std::array<std::optional<vgl::Image_info>, 6> face_textures;
                            for (auto f : files.value()) {
                                auto filename = f.filename().stem().string();
                                auto face_name = filename.substr(filename.size() - 2, filename.size());
                                if (face_name == "px") {
                                    face_textures.at(0) = vgl::Image_info{ f, 4 };
                                }
                                else if (face_name == "nx") {
                                    face_textures.at(1) = vgl::Image_info{ f, 4 };
                                }
                                else if (face_name == "py") {
                                    face_textures.at(2) = vgl::Image_info{ f, 4 };
                                }
                                else if (face_name == "ny") {
                                    face_textures.at(3) = vgl::Image_info{ f, 4 };
                                }
                                else if (face_name == "pz") {
                                    face_textures.at(4) = vgl::Image_info{ f, 4 };
                                }
                                else if (face_name == "nz") {
                                    face_textures.at(5) = vgl::Image_info{ f, 4 };
                                }
                            }
                            bool all_faces_present = true;
                            for (const auto& f : face_textures) {
                                all_faces_present &= f.has_value();
                            }
                            if (all_faces_present) {
                                cubemap_texture = vgl::gl::create_texture(GL_TEXTURE_CUBE_MAP);
                                auto desc = vgl::file::retrieve_image_desc(face_textures.at(0).value());
                                glTextureStorage2D(cubemap_texture, 6, desc.internal_format, desc.image_size.x, desc.image_size.y);
                                for (size_t i = 0; i < face_textures.size(); ++i) {
                                    const auto f = face_textures.at(i).value();
                                    auto filename = f.file_path.filename().stem().string();
                                    auto tex_data = vgl::file::load_image(f);
                                    std::cout << filename << "\n";
                                    vgl::gl::set_cubemap_data(cubemap_texture, tex_data, i);
                                }
                                glTextureParameteri(cubemap_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                glTextureParameteri(cubemap_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                glTextureParameteri(cubemap_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                                glTextureParameteri(cubemap_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                                glTextureParameteri(cubemap_texture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                                config.cm_conf.hdr = desc.type == GL_FLOAT;
                                vgl::gl::update_uniform(cubemap, 0, vgl::gl::get_texture_handle(cubemap_texture));
                                cubemap_config_ssbo = vgl::gl::create_buffer(config.cm_conf, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
                                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, cubemap_config_ssbo);
                                config.cubemap_equirectangular = false;
                            }
                        }
                    }
                }
                if (ImGui::MenuItem("Load equirectangular map as cubemap")) {
                    auto file = vgl::file::open_file_dialog(vgl::file::resources_path / "images", "jpg,png,hdr");
                    if (file) {
                        vgl::Image_info tex_info{ file.value(), 4 };
                        auto tex_data = vgl::file::load_image(tex_info);
                        cubemap_texture = vgl::gl::create_texture(GL_TEXTURE_2D);
                        glTextureStorage2D(cubemap_texture, 1, tex_data.desc.internal_format,
                            tex_data.desc.image_size.x, tex_data.desc.image_size.y);
                        vgl::gl::set_texture_data_2d(cubemap_texture, tex_data);
                        glTextureParameteri(cubemap_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTextureParameteri(cubemap_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTextureParameteri(cubemap_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTextureParameteri(cubemap_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glTextureParameteri(cubemap_texture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                        config.cm_conf.hdr = tex_data.desc.type == GL_FLOAT;
                        vgl::gl::update_uniform(cubemap_equirect, 0, vgl::gl::get_texture_handle(cubemap_texture));
                        cubemap_config_ssbo = vgl::gl::create_buffer(config.cm_conf, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
                        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, cubemap_config_ssbo);
                        config.cubemap_equirectangular = true;
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Reload Shaders")) {
                reload_shaders();
            }
        }
        ImGui::EndMainMenuBar();
        if (ImGui::Begin("Settings")) {
            ImGui::Checkbox("MSAA", &config.msaa);
            if (ImGui::CollapsingHeader("Cubemap settings")) {
                if (ImGui::Checkbox("Activate cubemap", &config.cubemap_active)) {

                }
                ImGui::Combo("Tonemapping", &config.cm_conf.tone_mapping, "Linear\0Reinhard\0Hejl and Burgess-Dawson\0Uncharted 2\0\0");
                ImGui::DragFloat("Gamma", &config.cm_conf.gamma, 0.01f);
                ImGui::DragFloat("Exposure", &config.cm_conf.exposure, 0.01f);
                if (config.cubemap_active) {
                    vgl::gl::update_full_buffer(cubemap_config_ssbo, config.cm_conf);
                }
            }
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Light settings")) {
                if (ImGui::Button("Add light")) {
                    config.lights.push_back(vgl::Light{});
                    config.lights_added_or_removed = true;
                }
                int i = 0;
                for (auto light_it = config.lights.begin(); light_it != config.lights.end();) {
                    if (ImGui::CollapsingHeader(("Light " + std::to_string(i)).c_str())) {
                        ImGui::Combo(("Type##light" + std::to_string(i)).c_str(), &light_it->type,
                            "Ambient\0Point\0Directional\0Spotlight\0\0");
                        ImGui::Text("Type: %d", light_it->type);
                        ImGui::ColorEdit3(("Color##light" + std::to_string(i)).c_str(),
                            glm::value_ptr(light_it->color), ImGuiColorEditFlags_Float);
                        ImGui::DragFloat(("Brightness##light" + std::to_string(i)).c_str(),
                            &light_it->color.a, 0.1f, 0.0f, 100.0f);
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
                            light_it = config.lights.erase(light_it);
                            config.lights_added_or_removed = true;
                        }
                    }
                    if (config.lights.end() == light_it) {
                        break;
                    }
                    ++light_it;
                    ++i;
                }
            }
        }
        ImGui::End();

        if (config.lights_added_or_removed) {
            glFinish();
            lights_ssbo = vgl::gl::create_buffer(config.lights, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lights_ssbo);
            config.lights_added_or_removed = false;
        }
        if (config.mesh_loaded) {
            vgl::gl::update_full_buffer(lights_ssbo, config.lights);
            if (!ImGui::IsAnyItemHovered() && !ImGui::IsAnyWindowHovered() && !ImGui::IsAnyItemFocused()
                && !ImGui::IsAnyItemActive()) {
                glm::vec3 dir(static_cast<float>(window.key[GLFW_KEY_D]) - static_cast<float>(window.key[GLFW_KEY_A]),
                    static_cast<float>(window.key[GLFW_KEY_E]) - static_cast<float>(window.key[GLFW_KEY_Q]),
                    static_cast<float>(window.key[GLFW_KEY_S]) - static_cast<float>(window.key[GLFW_KEY_W]));
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
                    if (config.cubemap_equirectangular) {
                        vgl::gl::update_uniform(cubemap_equirect, 0, vgl::gl::get_texture_handle(cubemap_texture));
                    }
                    else {
                        vgl::gl::update_uniform(cubemap, 0, vgl::gl::get_texture_handle(cubemap_texture));
                    }
                }
                vgl::gl::update_full_buffer(cam_ssbo, cam.get_cam_data());
            }
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glBindFramebuffer(GL_FRAMEBUFFER, g_buffer_one.fbo);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            glBindVertexArray(model_vao);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_indirect_buffer);
            glUseProgram(depth_prepass);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                static_cast<unsigned int>(scene.objects.size()), 0);
            glDepthFunc(GL_EQUAL);
            glBindVertexArray(model_vao);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_indirect_buffer);
            glUseProgram(phong);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                static_cast<unsigned int>(scene.objects.size()), 0);
            if (config.cubemap_active) {
                glDepthFunc(GL_LEQUAL);
                if (config.cubemap_equirectangular) {
                    glUseProgram(cubemap_equirect);
                }
                else {
                    glUseProgram(cubemap);
                }
                glBindVertexArray(cube_vao);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
            }
            glDepthFunc(GL_LESS);
            glUseProgram(lights_debug);
            glBindVertexArray(cube_vao);
            glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 14, static_cast<int>(config.lights.size()), 0);
            glDisable(GL_DEPTH_TEST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            glBindVertexArray(screen_vao);
            glProgramUniformHandleui64ARB(screen, glGetUniformLocation(screen, "tex"), vgl::gl::get_texture_handle(g_buffer_one.color));
            glUseProgram(screen);
            glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 1, 0);
            glEnable(GL_DEPTH_TEST);
            /*glBindVertexArray(debug_vao);
            glUseProgram(aabb_debug);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawArraysInstancedBaseInstance(GL_LINES, 0, 2, static_cast<int>(scene.object_bounds.size()), 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);*/
        }
        gui.render();
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
