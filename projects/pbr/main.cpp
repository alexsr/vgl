#include "vgl/control/window.hpp"
#include "vgl/rendering/scene.hpp"
#include "vgl/file/file.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/gpu_api/gl/buffer.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include "vgl/gpu_api/gl/texture.hpp"
#include "vgl/gpu_api/gl/framebuffer.hpp"
#include "vgl/gpu_api/gl/debug.hpp"
#include <chrono>
#include "glm/gtc/type_ptr.hpp"
#include <random>
#include <glsp/preprocess.hpp>
#include "vgl/control/gui.hpp"
#include "vgl/rendering/camera.hpp"
#include "vgl/rendering/light.hpp"
#include "demo_util.hpp"

// enable optimus!
using DWORD = uint32_t;

extern "C" {
_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

int main() {
    auto w_res = glm::ivec2(1600, 900);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
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

    vgl::ui::Gui gui(window);

    struct Demo_config {
        std::vector<vgl::Light> lights{ vgl::Light{glm::vec4(0, 0, 0, 1.0), glm::vec4(1.0), glm::vec4(0.0, -1.0, 0.0, 0.0),
            vgl::Attenuation{}, glm::pi<float>() / 4.0f, 1, 1, 0} };
        glm::ivec2 fb_res;
        bool mesh_loaded = false;
        bool lights_added_or_removed = false;
        float scale = 1.0f;
    } config;
    glfwGetFramebufferSize(window.get(), &config.fb_res.x, &config.fb_res.y);

    vgl::Camera cam;
    cam.projection = glm::perspective(glm::radians(60.0f), w_res.x / static_cast<float>(w_res.y), 0.001f, 100.0f);

    auto cam_ssbo = vgl::gl::create_buffer(cam.get_cam_data(), GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cam_ssbo);

    vgl::gl::glprogram cubemapping, lights_debug, cubemap, cubemap_equirect;

    auto reload_shaders = [&]() {
        auto cubemapping_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "shading/cubemapping.vert").contents;
        auto cubemapping_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "shading/cubemapping.frag").contents;
        auto cubemapping_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, cubemapping_vs_source);
        auto cubemapping_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, cubemapping_fs_source);
        cubemapping = vgl::gl::create_program({ cubemapping_vs, cubemapping_fs });

        auto cubemap_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "util/cubemap.vert").contents;
        auto cubemap_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "util/cubemap.frag").contents;
        auto cubemap_equirect_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "util/cubemap_equirectangular.frag").contents;
        auto cubemap_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, cubemap_vs_source);
        auto cubemap_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, cubemap_fs_source);
        auto cubemap_equirect_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, cubemap_equirect_fs_source);
        cubemap = vgl::gl::create_program({ cubemap_vs, cubemap_fs });
        cubemap_equirect = vgl::gl::create_program({ cubemap_vs, cubemap_equirect_fs });

        auto lights_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "debug/lights.vert").contents;
        auto lights_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "debug/lights.frag").contents;
        auto lights_debug_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, lights_vs_source);
        auto lights_debug_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, lights_fs_source);
        lights_debug = vgl::gl::create_program({ lights_debug_vs, lights_debug_fs });
    };
    reload_shaders();

    window.cbs.window_size["resize"] = [&](GLFWwindow*, int x, int y) {
        w_res.x = x;
        w_res.y = y;
    };

    window.cbs.framebuffer_size["resize"] = [&](GLFWwindow*, int x, int y) {
        if (x > 0 && y > 0 && (x != config.fb_res.x || y != config.fb_res.y)) {
            config.fb_res.x = x;
            config.fb_res.y = y;
            glViewport(0, 0, x, y);
            cam.change_aspect_ratio(x / static_cast<float>(y));
            vgl::gl::update_full_buffer(cam_ssbo, cam.get_cam_data());
        }
    };

    struct Cubemap_config {
        bool hdr = false;
        float gamma = 2.2f;
        float exposure = 1.0f;
        int tone_mapping = 0;
        bool cubemap_equirectangular = false;
        bool cubemap_loaded = false;
    } cm_conf;

    auto cube_vao = vgl::gl::create_vertex_array();
    auto cube_buffer = vgl::gl::create_buffer(vgl::geo::unit_cube);
    glVertexArrayVertexBuffer(cube_vao, 0, cube_buffer, 0, sizeof(glm::vec3));
    glEnableVertexArrayAttrib(cube_vao, 0);
    glVertexArrayAttribFormat(cube_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(cube_vao, 0, 0);

    auto cubemap_config_ssbo = vgl::gl::create_buffer(cm_conf, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, cubemap_config_ssbo);

    vgl::gl::gltexture cubemap_texture;

    auto cubemap_gui = [&]() {
        if (ImGui::Begin("Cubemap")) {
            if (ImGui::Button("Load cubemap faces")) {
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
                            cm_conf.hdr = desc.type == GL_FLOAT;
                            vgl::gl::update_uniform(cubemap, 0, vgl::gl::get_texture_handle(cubemap_texture));
                            cubemap_config_ssbo = vgl::gl::create_buffer(cm_conf, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
                            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, cubemap_config_ssbo);
                            cm_conf.cubemap_equirectangular = false;
                            cm_conf.cubemap_loaded = true;
                        }
                    }
                }
            }
            if (ImGui::Button("Load equirectangular map as cubemap")) {
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
                    cm_conf.hdr = tex_data.desc.type == GL_FLOAT;
                    vgl::gl::update_uniform(cubemap_equirect, 0, vgl::gl::get_texture_handle(cubemap_texture));
                    cubemap_config_ssbo = vgl::gl::create_buffer(cm_conf, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, cubemap_config_ssbo);
                    cm_conf.cubemap_equirectangular = true;
                    cm_conf.cubemap_loaded = true;
                }
            }
            ImGui::Combo("Tonemapping", &cm_conf.tone_mapping, "Linear\0Reinhard\0Hejl and Burgess-Dawson\0Uncharted 2\0\0");
            ImGui::DragFloat("Gamma", &cm_conf.gamma, 0.01f);
            ImGui::DragFloat("Exposure", &cm_conf.exposure, 0.01f);
            if (cm_conf.cubemap_loaded) {
                vgl::gl::update_full_buffer(cubemap_config_ssbo, cm_conf);
            }
        }
        ImGui::End();
    };

    auto debug_vao = vgl::gl::create_vertex_array();

    vgl::Scene scene{};
    scene = vgl::load_scene(vgl::file::resources_path / "models/sphere/sphere-cylcoords-4k.obj", true);
    while (length(scene.scene_bounds.min) > glm::sqrt(3.0f)) {
        for (auto& v : scene.vertices) {
            v.pos *= 0.1f;
        }
        scene.scene_bounds.min *= 0.1f;
        scene.scene_bounds.max *= 0.1f;
    }
    for (int i = -1; i < 2; ++i) {
        for (int j = -1; j < 2; ++j) {
            if (i == 0 && j == 0) continue;
            scene.objects.push_back(vgl::Scene_object{ glm::translate(scene.objects.at(0).model, 0.3f * glm::vec3(i, j, 0)), 0, -1, -1, -1, -1, -1, 0, 0 });
        }
    }
    scene.draw_cmds.at(0).instance_count = scene.objects.size();
    auto indices_buffer = vgl::gl::create_buffer(scene.indices);
    auto model_vbo = vgl::gl::create_buffer(scene.vertices);
    auto draw_indirect_buffer = vgl::gl::create_buffer(scene.draw_cmds);
    auto material_buffer = vgl::gl::create_buffer(scene.materials);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, material_buffer);
    auto scene_obj_buffer = vgl::gl::create_buffer(scene.objects, GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, scene_obj_buffer);
    auto bounds_buffer = vgl::gl::create_buffer(scene.object_bounds);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bounds_buffer);
    auto model_vao = vgl::gl::create_vertex_array();
    glVertexArrayVertexBuffer(model_vao, 0, model_vbo, 0,
        static_cast<int>(vgl::sizeof_value_type(scene.vertices)));
    glVertexArrayElementBuffer(model_vao, indices_buffer);
    glEnableVertexArrayAttrib(model_vao, 0);
    glEnableVertexArrayAttrib(model_vao, 1);
    glEnableVertexArrayAttrib(model_vao, 2);
    glEnableVertexArrayAttrib(model_vao, 3);
    glVertexArrayAttribFormat(model_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(vgl::Vertex, pos));
    glVertexArrayAttribFormat(model_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(vgl::Vertex, normal));
    glVertexArrayAttribFormat(model_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(vgl::Vertex, uv));
    glVertexArrayAttribFormat(model_vao, 3, 3, GL_FLOAT, GL_FALSE, offsetof(vgl::Vertex, tangent));
    glVertexArrayAttribBinding(model_vao, 0, 0);
    glVertexArrayAttribBinding(model_vao, 1, 0);
    glVertexArrayAttribBinding(model_vao, 2, 0);
    glVertexArrayAttribBinding(model_vao, 3, 0);
    cam.move_speed = sqrt(length((scene.scene_bounds.min - scene.scene_bounds.max) / 2.0f));

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
        cubemap_gui();
        if (ImGui::Begin("Settings")) {
            if (ImGui::CollapsingHeader("Light settings", ImGuiTreeNodeFlags_DefaultOpen)) {
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
        vgl::gl::update_full_buffer(lights_ssbo, config.lights);
        if (!ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive()) {
            if (window.key[GLFW_KEY_R]) {
                reload_shaders();
                if (cm_conf.cubemap_equirectangular) {
                    vgl::gl::update_uniform(cubemap_equirect, 0, vgl::gl::get_texture_handle(cubemap_texture));
                }
                else {
                    vgl::gl::update_uniform(cubemap, 0, vgl::gl::get_texture_handle(cubemap_texture));
                }
            }
        }
        if (cm_conf.cubemap_loaded) {
            if (cm_conf.cubemap_equirectangular) {
                vgl::gl::update_uniform(cubemapping, 1, vgl::gl::get_texture_handle(cubemap_texture));
                vgl::gl::update_uniform(cubemapping, 2, true);
            }
            else {
                vgl::gl::update_uniform(cubemapping, 0, vgl::gl::get_texture_handle(cubemap_texture));
                vgl::gl::update_uniform(cubemapping, 2, false);
            }
        }
        demo::update_cam(cam, window, dt);
        scene.objects.at(0).model = glm::rotate(scene.objects.at(0).model, glm::radians(20.0f * dt), glm::vec3(0, 1, 0));
        vgl::gl::update_full_buffer(scene_obj_buffer, scene.objects);
        vgl::gl::update_full_buffer(cam_ssbo, cam.get_cam_data());
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glBindVertexArray(model_vao);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_indirect_buffer);
        glUseProgram(cubemapping);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
            static_cast<unsigned int>(scene.draw_cmds.size()), 0);
        if (cm_conf.cubemap_loaded) {
            glDepthFunc(GL_LEQUAL);
            if (cm_conf.cubemap_equirectangular) {
                glUseProgram(cubemap_equirect);
            }
            else {
                glUseProgram(cubemap);
            }
            glBindVertexArray(cube_vao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
            glDepthFunc(GL_LESS);
        }
        gui.render();
        window.swap_buffers();
        demo::hide_cursor(window);
        window.poll_events();
    }
}
