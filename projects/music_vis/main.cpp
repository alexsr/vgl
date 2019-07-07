#include <stdlib.h>
#include <iostream>
#include "vgl/file/file.hpp"
#include "vgl/control/window.hpp"
#include "vgl/control/gui.hpp"
#include "vgl/gpu_api/gl/debug.hpp"
#include "vgl/gpu_api/gl/buffer.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/gpu_api/gl/texture.hpp"
#include "vgl/gpu_api/gl/framebuffer.hpp"
#include "vgl/rendering/geometry.hpp"
#include "vgl/rendering/camera.hpp"
#include <glsp/glsp.hpp>
#include "vgl/rendering/scene.hpp"
#include <random>
#include "audio_input.hpp"
#include "processing.hpp"
#include <complex>
#include <algorithm>
#include <execution>
#include <functional>

// enable optimus!

extern "C" {
    _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
}

std::pair<std::vector<unsigned int>, std::vector<glm::vec2>> gen_mesh_xz(int res_x, int res_z) {
    std::vector<glm::vec2> vertices(res_x * res_z);
    std::vector<unsigned int> indices(2 * res_x * (res_z - 1) + 2 * (res_z - 2));
    for (int i = 0; i < res_z; ++i) {
        for (int j = 0; j < res_x; ++j) {
            float x = j;
            float z = i;
            vertices.at(i * res_x + j) = glm::vec2(x, z);
        }
    }
    int id = 0;
    for (int i = 0; i < res_z - 1; ++i) {
        for (int j = 0; j < res_x; ++j) {
            indices.at(id++) = i * res_x + j;
            indices.at(id++) = (i + 1) * res_x + j;
        }
        if (i < res_z - 2) {
            indices.at(id++) = (i + 1) * res_x + res_x - 1;
            indices.at(id++) = i * res_x + res_x;
        }
    }
    return {indices, vertices};
}

int main() {
    auto fb_res = glm::ivec2(1600, 1600.0 / 4.0 * 3.0);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    vgl::Window window(fb_res.x, fb_res.y, "Hello", glfwGetPrimaryMonitor());
    window.enable_gl();
    glfwSwapInterval(0);
    glViewport(0, 0, fb_res.x, fb_res.y);
    glClearColor(0.749f, 0.067f, 0.706f, 0.0f);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(vgl::gl::debug_callback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    vgl::ui::Gui gui(window);

    struct Vis_settings {
        glm::vec2 mesh_size;
        glm::ivec2 grid_res;
    } vis_settings{ {0.8f, 2.0f},{138, 256} };
    auto vis_settings_ssbo = vgl::gl::create_buffer(vis_settings);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, vis_settings_ssbo);

    vgl::Camera cam;
    cam.rotation = glm::angleAxis(glm::radians(-10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    //cam.rotation *= glm::angleAxis(glm::radians(-180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    cam.position = glm::vec4(0.0f, 0.02f, vis_settings.mesh_size.y + 0.005, 1.0f);
    cam.projection = glm::perspective(glm::radians(90.0f), fb_res.x / static_cast<float>(fb_res.y), 0.01f, 100.0f);
    auto cam_ssbo = vgl::gl::create_buffer(cam.get_cam_data(), GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cam_ssbo);

    //auto car_img = vgl::file::load_image(vgl::file::resources_path / "images/car_back_window_1.jpg");

    float fs = 48000.0f;
    unsigned int block_length = 1024; // 2^10
    auto half_block = block_length / 2; // 2^10
    [[maybe_unused]] auto fn = fs / 2.0f; // Bandwidth fn = Nyquist frequency
    auto measurement_duration = block_length / fs; // duration of block in seconds
    [[maybe_unused]] auto frequency_resolution = fs / static_cast<float>(block_length);
    impl::Recorder rec(fs, block_length);
    std::vector<float> wavebuffer(rec.data.size());
    Exp_moving_average ema_wavebuffer(std::sqrt(frequency_resolution));

    auto block_f = static_cast<float>(block_length);
    std::vector<float> fft_mag(half_block, 0.0f);
    auto fft_buffer = vgl::gl::create_buffer(fft_mag, GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, fft_buffer);
    auto fft_ptr = glMapNamedBufferRange(fft_buffer, 0, std::size(fft_mag) * sizeof(float),
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    Exp_moving_average_buffer ema_fft(std::sqrt(frequency_resolution), fft_mag.size());

    Ring_buffer heights(vis_settings.grid_res.x * vis_settings.grid_res.y);
    auto height_map = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureStorage2D(height_map, 1, GL_R32F, vis_settings.grid_res.x, vis_settings.grid_res.y);
    glTextureParameteri(height_map, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(height_map, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(height_map, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(height_map, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(height_map, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    vgl::gl::set_texture_data_2d(height_map, heights.data, vis_settings.grid_res.x, vis_settings.grid_res.y, GL_RED);
    auto height_handle = vgl::gl::get_texture_handle(height_map);
    Exp_moving_average_buffer ema_new_heights(4, vis_settings.grid_res.x);

    auto noise_texture = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureStorage2D(noise_texture, 1, GL_R32F, 480, 320);
    glTextureParameteri(noise_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(noise_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(noise_texture, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(noise_texture, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureParameteri(noise_texture, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
    {
        std::vector<float> noise_data(480 * 320);
        std::random_device rd;
        std::mt19937 eng(rd());
        std::normal_distribution<float> normal_distri(0.0f, 1.0f);
        std::generate(noise_data.begin(), noise_data.end(), [&normal_distri, &rd, &eng]() {
            return normal_distri(eng); });
        vgl::gl::set_texture_data_2d(noise_texture, noise_data, 480, 320, GL_RED);
    }
    auto noise_handle = vgl::gl::get_texture_handle(noise_texture);


    struct Billboard_texture {
        GLuint64 texture;
        float aspect_ratio;
    };

    std::vector<vgl::gl::gltexture> billboard_gl_tex(5);
    std::vector<Billboard_texture> billboard_textures;
    for (int i = 0; i < 5; ++i) {
        auto palm_tree_one_stbi = vgl::file::load_image(vgl::file::resources_path / ("images/music_vis/palm_tree_"
            + std::to_string(i + 1) + ".png"), 4, true);
        auto palm_tree_one_desc = vgl::img::get_image_desc(palm_tree_one_stbi);
        billboard_gl_tex.at(i) = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureStorage2D(billboard_gl_tex.at(i), 1, vgl::gl::derive_internal_format(palm_tree_one_stbi),
            palm_tree_one_desc.width, palm_tree_one_desc.height);
        glTextureParameteri(billboard_gl_tex.at(i), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(billboard_gl_tex.at(i), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(billboard_gl_tex.at(i), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(billboard_gl_tex.at(i), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        vgl::gl::set_texture_data_2d(billboard_gl_tex.at(i), palm_tree_one_stbi);
        billboard_textures.push_back({ vgl::gl::get_texture_handle(billboard_gl_tex.at(i)),
            palm_tree_one_desc.width / static_cast<float>(palm_tree_one_desc.height) });
    }

    std::vector<GLuint64> iced_tea_handles;
    std::vector<vgl::gl::gltexture> iced_tea_tex(5);
    for (int i = 0; i < 4; ++i) {
        auto palm_tree_one_stbi = vgl::file::load_image(vgl::file::resources_path / ("images/music_vis/arizona_"
            + std::to_string(i + 1) + ".png"), 4);
        auto palm_tree_one_desc = vgl::img::get_image_desc(palm_tree_one_stbi);
        iced_tea_tex.at(i) = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureStorage2D(iced_tea_tex.at(i), 1, vgl::gl::derive_internal_format(palm_tree_one_stbi),
            palm_tree_one_desc.width, palm_tree_one_desc.height);
        glTextureParameteri(iced_tea_tex.at(i), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(iced_tea_tex.at(i), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(iced_tea_tex.at(i), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(iced_tea_tex.at(i), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        vgl::gl::set_texture_data_2d(iced_tea_tex.at(i), palm_tree_one_stbi);
        iced_tea_handles.push_back(vgl::gl::get_texture_handle(iced_tea_tex.at(i)));
    }


    auto fiji_stbi = vgl::file::load_image(vgl::file::resources_path / ("images/music_vis/fiji.png"), 4);
    auto fiji_desc = vgl::img::get_image_desc(fiji_stbi);
    iced_tea_tex.at(4) = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureStorage2D(iced_tea_tex.at(4), 1, vgl::gl::derive_internal_format(fiji_stbi),
        fiji_desc.width, fiji_desc.height);
    glTextureParameteri(iced_tea_tex.at(4), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(iced_tea_tex.at(4), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(iced_tea_tex.at(4), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(iced_tea_tex.at(4), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    vgl::gl::set_texture_data_2d(iced_tea_tex.at(4), fiji_stbi);
    iced_tea_handles.push_back(vgl::gl::get_texture_handle(iced_tea_tex.at(4)));

    auto iced_tea_ssbo = vgl::gl::create_buffer(iced_tea_handles);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, iced_tea_ssbo);

    struct Billboard_def {
        Billboard_def(glm::vec2 p, glm::vec2 s) : pos(p), scale(s) {};
        glm::vec2 pos;
        glm::vec2 scale;
        float noise_offset = 0.0f;
        float height_noise = 0.0f;
        int pad1, pad2;
    };

    std::vector<Billboard_def> billboards;
    billboards.push_back({ glm::vec2(0.5, 2.0f), glm::vec2(-0.1, 0.1)});
    billboards.push_back({ glm::vec2(0.5, 1.94f), glm::vec2(0.1, 0.1)});
    billboards.push_back({ glm::vec2(0.5, 1.7f), glm::vec2(-0.1, 0.1)});
    billboards.push_back({ glm::vec2(0.5, 1.4f), glm::vec2(0.1, 0.1)});
    billboards.push_back({ glm::vec2(0.5, 1.33f), glm::vec2(-0.1, 0.1)});
    billboards.push_back({ glm::vec2(0.5, 1.0f), glm::vec2(0.1, 0.1)});
    billboards.push_back({ glm::vec2(0.5, 0.884f), glm::vec2(-0.1, 0.1)});
    billboards.push_back({ glm::vec2(0.5, 0.7f), glm::vec2(0.1, 0.1)});
    billboards.push_back({ glm::vec2(0.5, 0.6f), glm::vec2(-0.1, 0.1) });
    billboards.push_back({ glm::vec2(0.5, 0.455f), glm::vec2(0.1, 0.1) });
    billboards.push_back({ glm::vec2(0.5, 0.3f), glm::vec2(-0.1, 0.1) });
    billboards.push_back({ glm::vec2(0.5, 0.14f), glm::vec2(0.1, 0.1) });

    auto billboard_ssbo = vgl::gl::create_buffer(billboards);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, billboard_ssbo);

    auto billboard_texture_ssbo = vgl::gl::create_buffer(billboard_textures);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, billboard_texture_ssbo);


    auto screen_vao = vgl::gl::create_vertex_array();

    auto mesh = gen_mesh_xz(vis_settings.grid_res.x, vis_settings.grid_res.y);
    auto mesh_vao = vgl::gl::create_vertex_array();
    auto mesh_buffer = vgl::gl::create_buffer(mesh.second);
    auto mesh_ibo = vgl::gl::create_buffer(mesh.first);
    glVertexArrayElementBuffer(mesh_vao, mesh_ibo);
    glVertexArrayVertexBuffer(mesh_vao, 0, mesh_buffer, 0, sizeof(glm::vec2));
    glEnableVertexArrayAttrib(mesh_vao, 0);
    glVertexArrayAttribFormat(mesh_vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(mesh_vao, 0, 0);

    auto fb_postprocess = vgl::gl::create_framebuffer();
    vgl::gl::gltexture color_postprocess;
    vgl::gl::gltexture depth_postprocess;
    auto reset_fb = [&]() {
        auto size = window.framebuffer_size();
        color_postprocess = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(color_postprocess, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(color_postprocess, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(color_postprocess, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTextureParameteri(color_postprocess, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
        glTextureStorage2D(color_postprocess, 1, GL_RGBA32F, size.x, size.y);
        glNamedFramebufferTexture(fb_postprocess, GL_COLOR_ATTACHMENT0, color_postprocess, 0);

        depth_postprocess = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(depth_postprocess, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(depth_postprocess, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(depth_postprocess, 1, GL_DEPTH24_STENCIL8, size.x, size.y);
        glNamedFramebufferTexture(fb_postprocess, GL_DEPTH_ATTACHMENT, depth_postprocess, 0);
        vgl::gl::attach_draw_buffers(fb_postprocess, { GL_COLOR_ATTACHMENT0 });
        vgl::gl::check_framebuffer(fb_postprocess);
    };
    reset_fb();

    struct Particle {
        glm::vec4 pos;
        glm::vec4 vel;
        glm::vec4 accel;
        glm::vec3 color;
        float lifetime{ 0.0f };
    };

    struct Particle_system_config {
        int update_count = 0;
        int offset = 0;
        float speed_scale = 0.1f;
        float lifetime_scale = 0.1f;
        float point_size = 1.0;
    };

    Particle_system_config tea_system_config;
    Particle_system_config water_system_config;
    water_system_config.point_size = 3.8f;
    water_system_config.lifetime_scale = 1.1f;
    water_system_config.speed_scale = 0.420f;

    water_system_config.update_count = half_block / 2;
    tea_system_config.update_count = 16;
    tea_system_config.lifetime_scale = 1.1f;
    tea_system_config.speed_scale = 0.020f;

    auto water_particle_count = water_system_config.update_count * 16;
    auto tea_particle_count = tea_system_config.update_count * 32;

    vgl::gl::glprogram move_particles;
    vgl::gl::glprogram spawn_water_particles;
    vgl::gl::glprogram spawn_iced_tea_particles;
    vgl::gl::glprogram render_particles;
    vgl::gl::glprogram render_iced_tea;
    vgl::gl::glprogram render_mesh;
    vgl::gl::glprogram render_billboards;
    vgl::gl::glprogram background;
    vgl::gl::glprogram postprocess;

    auto reload_shader = [&]() {
        auto vs_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/render_water_particles.vert").contents;
        auto fs_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/render_particles.frag").contents;
        auto comp_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/move_particles.comp").contents;
        auto iced_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/render_iced_tea.vert").contents;
        auto iced_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/render_iced_tea.frag").contents;
        auto spawn_water_src = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/spawn_water_particles.comp").contents;
        auto spawn_iced_tea_src = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/spawn_iced_tea_particles.comp").contents;
        auto particles_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, vs_source);
        auto particles_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, fs_source);
        render_particles = vgl::gl::create_program({ particles_vs, particles_fs });
        auto iced_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, iced_vs_source);
        auto iced_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, iced_fs_source);
        render_iced_tea = vgl::gl::create_program({ iced_vs, iced_fs });
        auto move_cs = vgl::gl::create_shader(GL_COMPUTE_SHADER, comp_source);
        move_particles = vgl::gl::create_program({ move_cs });
        auto spawn_cs = vgl::gl::create_shader(GL_COMPUTE_SHADER, spawn_iced_tea_src);
        spawn_iced_tea_particles = vgl::gl::create_program({ spawn_cs });
        auto spawn_water_cs = vgl::gl::create_shader(GL_COMPUTE_SHADER, spawn_water_src);
        spawn_water_particles = vgl::gl::create_program({ spawn_water_cs });
        auto mesh_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/mesh.vert").contents;
        auto mesh_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/mesh.frag").contents;
        auto mesh_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, mesh_vs_source);
        auto mesh_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, mesh_fs_source);
        render_mesh = vgl::gl::create_program({ mesh_vs, mesh_fs });
        auto billboards_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/billboards.vert").contents;
        auto billboards_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/billboards.frag").contents;
        auto billboards_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, billboards_vs_source);
        auto billboards_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, billboards_fs_source);
        render_billboards = vgl::gl::create_program({ billboards_vs, billboards_fs });
        auto pp_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "minimal/texture.vert").contents;
        auto pp_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/vcr.frag").contents;
        auto pp_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, pp_vs_source);
        auto pp_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, pp_fs_source);
        auto background_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/background.frag").contents;
        auto background_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, background_fs_source);
        postprocess = vgl::gl::create_program({ pp_vs, pp_fs });
        background = vgl::gl::create_program({ pp_vs, background_fs });
        vgl::gl::update_uniform(render_mesh, 0, height_handle);
        vgl::gl::update_uniform(render_mesh, 1, noise_handle);
        vgl::gl::update_uniform(render_billboards, 1, noise_handle);
        vgl::gl::update_uniform(postprocess, 0, vgl::gl::get_texture_handle(color_postprocess));
        vgl::gl::update_uniform(postprocess, 1, noise_handle);
        vgl::gl::update_uniform(postprocess, 3, fb_res.x / static_cast<float>(fb_res.y));
        vgl::gl::update_uniform(background, 0, fb_res.x / static_cast<float>(fb_res.y));
        vgl::gl::update_uniform(render_iced_tea, 0, water_particle_count);
        vgl::gl::update_uniform(spawn_iced_tea_particles, 0, noise_handle);
        vgl::gl::update_uniform(spawn_iced_tea_particles, 2, water_particle_count);
    };
    reload_shader();

    window.cbs.framebuffer_size["resize"] = [&](GLFWwindow*, int x, int y) {
        if (x > 0 && y > 0 && (x != fb_res.x || y != fb_res.y)) {
            fb_res.x = x;
            fb_res.y = y;
            reset_fb();
            glViewport(0, 0, x, y);
            cam.change_aspect_ratio(x / static_cast<float>(y));
            vgl::gl::update_full_buffer(cam_ssbo, cam.get_cam_data());
            vgl::gl::update_uniform(postprocess, 3, x / static_cast<float>(y));
            vgl::gl::update_uniform(postprocess, 0, vgl::gl::get_texture_handle(color_postprocess));
            vgl::gl::update_uniform(background, 0, fb_res.x / static_cast<float>(fb_res.y));
        }
    };

    auto particle_count = water_particle_count + tea_particle_count;
    auto size_p = particle_count * sizeof(Particle);
    auto p_ssbo = vgl::gl::create_buffer_fixed_size(size_p, GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, p_ssbo);
    auto vao = vgl::gl::create_vertex_array();
    auto config_ssbo = vgl::gl::create_buffer(tea_system_config, GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);
    const auto config_ptr = glMapNamedBufferRange(config_ssbo, 0, sizeof(tea_system_config),
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, config_ssbo);

    auto sky_config_ssbo = vgl::gl::create_buffer(water_system_config, GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);
    const auto sky_config_ptr = glMapNamedBufferRange(sky_config_ssbo, 0, sizeof(water_system_config),
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, sky_config_ssbo);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SPRITE);
    float chromatic_abboration = 0.0f;

    using high_res_clock = std::chrono::high_resolution_clock;
    auto current = high_res_clock::now();
    auto previous = high_res_clock::now();
    float dt = 0.0f;
    float update_time = 0.0f;
    bool pause = false;
    std::array<int, 3> wgs{ 1, 1, 1 };
    impl::Volume_getter vg;
    float curr_time = 0.0f;
    float offset = 0;
    float log_base = std::log(50000.0f);

    float steps_per_bin = 10;
    std::vector<float> temp_heights(vis_settings.grid_res.x, 0.0f);
    float eps_planck_taper = 0.1;
    auto max_n = static_cast<int>(temp_heights.size()) - 15;
    auto bin_size = (fft_mag.size() / 2 - 1) / static_cast<float>(max_n);
    auto bin_step_size = bin_size / steps_per_bin;
    int eps_n = static_cast<int>(eps_planck_taper * max_n);
    float one_minus_eps_n = (1.0f - eps_planck_taper) * max_n;
    int one_minus_eps_n_floored = static_cast<int>(one_minus_eps_n);
    auto z = [](float eps, float n, float i, auto binary_op) {
        return 2.0f * eps * (1.0f / binary_op(1.0f, 2.0f * i / n - 1.0f) + 1.0f / binary_op(1.0f - 2.0f * eps, 2.0f * i / n - 1));
    };

    std::vector<float> planck_taper_plus(eps_n);
    for (int i = 0; i < eps_n; ++i) {
        planck_taper_plus.at(i) = 1.0f / (std::exp(z(eps_planck_taper,
            static_cast<float>(max_n), static_cast<float>(i), std::plus{})) + 1.0f);
    }
    std::vector<float> planck_taper_minus(max_n - one_minus_eps_n_floored - 1);
    for (int i = static_cast<int>(one_minus_eps_n) + 1; i < max_n; ++i) {
        planck_taper_minus.at(i - one_minus_eps_n_floored - 1) = 1.0f / (std::exp(z(eps_planck_taper,
            static_cast<float>(max_n), static_cast<float>(i), std::minus{})) + 1.0f);
    }

    auto spawn_iced_tea_lambda = [&]() {
        if (tea_system_config.offset + tea_system_config.update_count >= tea_particle_count) {
            tea_system_config.offset = 0;
        }
        std::memcpy(config_ptr, &tea_system_config, sizeof(tea_system_config));

        glUseProgram(spawn_iced_tea_particles);
        glGetProgramiv(spawn_iced_tea_particles, GL_COMPUTE_WORK_GROUP_SIZE, wgs.data());
        glDispatchCompute(glm::ceil(tea_system_config.update_count / static_cast<float>(wgs.at(0))), 1, 1);
        tea_system_config.offset += tea_system_config.update_count;
    };
    
    while (!window.should_close()) {
        //gui.start_frame();
        dt = std::chrono::duration<float>(current - previous).count();
        update_time += dt;
        curr_time += dt;
        previous = current;
        current = high_res_clock::now();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        /*system_config.rot = glm::mat4(glm::cos(acc), 0, -glm::sin(acc), 0, 0, 1.0f, 0, 0, glm::sin(acc), 0, glm::cos(acc), 0, 0, 0, 0, 1);
        acc += dt * 0.1f * glm::pi<float>();*/
        if (!pause && update_time > measurement_duration) {
            //std::cout << "U:" << update_time << "; M:" << measurement_duration << "; dt:" << dt << "\n";
            update_time -= measurement_duration;
            //if (rec.receive()) {
            {
                std::lock_guard<std::mutex> lock(rec.audio_data_mutex);
                std::copy(rec.data.begin(), rec.data.end(), wavebuffer.begin());
            }
            auto min_max_wave = std::minmax_element(wavebuffer.begin(), wavebuffer.end());
            auto max_abs = glm::max(glm::abs(*min_max_wave.first), glm::abs(*min_max_wave.second));
            //ema.add(max_abs);
            //auto max_avg = ema.ema();
            auto volume = vg.get_volume();
            if (/*max_avg > 0.001f && */max_abs > 0.001f && volume > 0.0f) {
                /*std::transform(std::execution::par, wavebuffer.begin(), wavebuffer.end(), wavebuffer.begin(), [volume](float x) {
                    return x / volume;
                    });*/
                auto bound = block_f - 1.0f;
                for (int t = 0; t < wavebuffer.size(); ++t) {
                    wavebuffer.at(t) *= hann_function(t, bound) / volume;
                }
                cooley_tukey_fft_mag(wavebuffer, fft_mag);
                fft_mag.at(0) = 0;
                ema_fft.add(fft_mag);
                for (int i = 0; i < eps_n; ++i) {
                    temp_heights.at(i) = 0.0f;
                    for (float b = 0; b < steps_per_bin; ++b) {
                        temp_heights.at(i) += fft_mag.at(i * bin_size + b * bin_step_size);
                    }
                    temp_heights.at(i) = std::log(temp_heights.at(i) / steps_per_bin + 1.0f) / log_base;
                    temp_heights.at(i) *= planck_taper_plus.at(i);
                }
                for (int i = eps_n; i < one_minus_eps_n_floored; ++i) {
                    temp_heights.at(i) = 0.0f;
                    for (float b = 0; b < steps_per_bin; ++b) {
                        temp_heights.at(i) += fft_mag.at(i * bin_size + b * bin_step_size);
                    }
                    temp_heights.at(i) = std::log(temp_heights.at(i) / steps_per_bin + 1.0f) / log_base;
                }
                for (int i = one_minus_eps_n_floored + 1; i < max_n; ++i) {
                    temp_heights.at(i) = 0.0f;
                    for (float b = 0; b < steps_per_bin; ++b) {
                        temp_heights.at(i) += fft_mag.at(i * bin_size + b * bin_step_size);
                    }
                    temp_heights.at(i) = std::log(temp_heights.at(i) / steps_per_bin + 1.0f) / log_base;
                    temp_heights.at(i) *= planck_taper_minus.at(i - one_minus_eps_n_floored - 1);
                }
                for (int i = max_n; i < temp_heights.size(); ++i) {
                    temp_heights.at(i) = 0.0f;
                }
                ema_new_heights.add(temp_heights);
                heights.push(ema_new_heights.ema);
                vgl::gl::update_uniform(spawn_iced_tea_particles, 1, curr_time);
                std::memcpy(fft_ptr, &ema_fft.ema, sizeof(float)* ema_fft.ema.size());
                if (ema_fft.ema.at(1) * 1.5 < fft_mag.at(1)) {
                    if (water_system_config.offset + water_system_config.update_count >= water_particle_count) {
                        water_system_config.offset = 0;
                    }
                    std::memcpy(sky_config_ptr, &water_system_config, sizeof(water_system_config));

                    glUseProgram(spawn_water_particles);
                    glGetProgramiv(spawn_water_particles, GL_COMPUTE_WORK_GROUP_SIZE, wgs.data());
                    glDispatchCompute(glm::ceil(water_system_config.update_count / static_cast<float>(wgs.at(0))), 1, 1);
                    water_system_config.offset += water_system_config.update_count;
                }
                if (ema_fft.ema.at(half_block * 0.3) * 3.4 < fft_mag.at(half_block * 0.3)) {
                    chromatic_abboration = 2.f;
                }
                if (ema_fft.ema.at(1) * 1.5 < fft_mag.at(1)) {
                    vgl::gl::update_uniform(spawn_iced_tea_particles, 3, 0.03f);
                    spawn_iced_tea_lambda();
                    vgl::gl::update_uniform(spawn_iced_tea_particles, 3, -0.03f);
                    spawn_iced_tea_lambda();
                }
                if (ema_fft.ema.at(half_block * 0.5) * 3.4 < fft_mag.at(half_block * 0.5)) {
                    vgl::gl::update_uniform(spawn_iced_tea_particles, 3, 0.0f);
                    spawn_iced_tea_lambda();
                }
                //std::memcpy(heights_ptr, heights.data.data(), heights.data.size() * sizeof(float));
            }
            else {
                heights.push(std::vector<float>(temp_heights.size()));
            }
            vgl::gl::set_texture_data_2d(height_map, heights.data, vis_settings.grid_res.x, vis_settings.grid_res.y, GL_RED);
            offset += 1.0f / vis_settings.grid_res.y;
            vgl::gl::update_uniform(render_mesh, 2, curr_time);
            vgl::gl::update_uniform(render_mesh, 3, offset);
            glBindFramebuffer(GL_FRAMEBUFFER, fb_postprocess);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDepthMask(GL_FALSE);
            glBindVertexArray(screen_vao);
            glUseProgram(background);
            glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, 1, 0);
            glDepthMask(GL_TRUE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBindVertexArray(mesh_vao);
            glUseProgram(render_mesh);
            glDrawElementsInstancedBaseInstance(GL_TRIANGLE_STRIP, mesh.first.size(), GL_UNSIGNED_INT, nullptr, 1, 0);
            vgl::gl::update_uniform(render_billboards, 0, offset);
            glBindVertexArray(screen_vao);
            glUseProgram(render_billboards);
            glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, billboards.size(), 0);
            glDisable(GL_BLEND);
        }
/*
        if (ImGui::Begin("Vis")) {
            if (ImGui::Button("Play / Pause")) pause = !pause;
            ImGui::DragFloat("Point size", &point_size, 0.05f);
            ImGui::DragFloat("Speed scale", &water_system_config.speed_scale, 0.01f);
            ImGui::DragFloat("Lifetime scale", &water_system_config.lifetime_scale, 0.01f);
            ImGui::DragFloat("Planck taper eps", &eps_planck_taper, 0.01f, 0.0f, 0.99f);
        }
        ImGui::End();*/

        glBindFramebuffer(GL_FRAMEBUFFER, fb_postprocess);
        glUseProgram(move_particles);
        glGetProgramiv(move_particles, GL_COMPUTE_WORK_GROUP_SIZE, wgs.data());
        vgl::gl::update_uniform(move_particles, 0, dt);
        glDispatchCompute(glm::ceil(particle_count / static_cast<float>(wgs.at(0))), 1, 1);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(vao);
        glUseProgram(render_particles);
        glDrawArraysInstancedBaseInstance(GL_POINTS, 0, 1, water_particle_count, 0);
        glUseProgram(render_iced_tea);
        glDrawArraysInstancedBaseInstance(GL_POINTS, 0, 1, tea_particle_count, 0);
        glDisable(GL_BLEND);

        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        vgl::gl::update_uniform(postprocess, 2, curr_time);
        vgl::gl::update_uniform(postprocess, 4, chromatic_abboration);
        chromatic_abboration -= 0.01;
        glUseProgram(postprocess);
        glBindVertexArray(screen_vao);
        glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 1, 0);
        glEnable(GL_DEPTH_TEST);

        //gui.render();

        if (!ImGui::IsAnyItemHovered() && !ImGui::IsAnyWindowHovered() && !ImGui::IsAnyItemFocused()
            && !ImGui::IsAnyItemActive()) {
            /*glm::vec3 dir(static_cast<float>(window.key[GLFW_KEY_D]) - static_cast<float>(window.key[GLFW_KEY_A]),
                static_cast<float>(window.key[GLFW_KEY_E]) - static_cast<float>(window.key[GLFW_KEY_Q]),
                static_cast<float>(window.key[GLFW_KEY_S]) - static_cast<float>(window.key[GLFW_KEY_W]));
            if (dot(dir, dir) != 0.0f) {
                cam.move(glm::normalize(dir), dt);
            }
            if (window.mouse[GLFW_MOUSE_BUTTON_LEFT]) {
                cam.rotate(glm::radians(glm::vec2(window.cursor_delta)), dt);
            }
            vgl::gl::update_full_buffer(cam_ssbo, cam.get_cam_data());*/
            /*if (window.key[GLFW_KEY_P]) {
                cam.reset();
            }*/
            if (window.key[GLFW_KEY_R]) {
                reload_shader();
            }
            /*if (window.mouse[GLFW_MOUSE_BUTTON_LEFT]) {
                glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else {
                glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }*/
        }
        window.swap_buffers();
        window.poll_events();
    }
}
