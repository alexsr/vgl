#include <stdlib.h>
#include <iostream>
#include "vgl/file/file.hpp"
#include "vgl/control/window.hpp"
#include "vgl/control/gui.hpp"
#include "vgl/gpu_api/gl/debug.hpp"
#include "vgl/rendering/camera.hpp"
#include "vgl/gpu_api/gl/buffer.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/rendering/geometry.hpp"
#include <glsp/glsp.hpp>
#include "vgl/rendering/scene.hpp"
#include <random>
#include "audio_input.hpp"

// enable optimus!

extern "C" {
    _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
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

float hann_function(float x, float bound) {
    auto res = glm::sin(glm::pi<float>() * x / (bound - 1));
    return res * res;
}

int main() {
    auto fb_res = glm::ivec2(1600, 900);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    vgl::Window window(fb_res.x, fb_res.y, "Hello");
    window.enable_gl();
    glfwSwapInterval(0);
    glViewport(0, 0, fb_res.x, fb_res.y);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(vgl::gl::debug_callback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    vgl::ui::Gui gui(window);
    vgl::Camera cam;
    cam.projection = glm::perspective(glm::radians(60.0f), fb_res.x / static_cast<float>(fb_res.y), 0.01f, 100.0f);
    cam.rotation_speed = 3.0f;

    auto cam_ssbo = vgl::gl::create_buffer(cam.get_cam_data(), GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cam_ssbo);
    window.cbs.framebuffer_size["resize"] = [&](GLFWwindow*, int x, int y) {
        if (x > 0 && y > 0 && (x != fb_res.x || y != fb_res.y)) {
            fb_res.x = x;
            fb_res.y = y;
            glViewport(0, 0, x, y);
            cam.change_aspect_ratio(x / static_cast<float>(y));
            vgl::gl::update_full_buffer(cam_ssbo, cam.get_cam_data());
        }
    };
    float fs = 44100.0f;
    unsigned int block_length = 256; // 2^10
    auto half_block = block_length / 2; // 2^10
    [[maybe_unused]] auto fn = fs / 2.0f; // Bandwidth fn = Nyquist frequency
    [[maybe_unused]] auto measurement_duration = block_length / fs; // duration of block in seconds
    [[maybe_unused]] auto frequency_resolution = fs / static_cast<float>(block_length);

    vgl::gl::glprogram move_particles;
    vgl::gl::glprogram spawn_particles;
    vgl::gl::glprogram render_particles;

    auto reload_shader = [&]() {
        auto vs_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/render_particles.vert").contents;
        auto fs_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/render_particles.frag").contents;
        auto comp_source = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/move_particles.comp").contents;
        auto spawn_src = glsp::preprocess_file(vgl::file::shaders_path / "music_vis/spawn_particles.comp").contents;
        auto particles_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, vs_source);
        auto particles_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, fs_source);
        render_particles = vgl::gl::create_program({ particles_vs, particles_fs });
        auto move_cs = vgl::gl::create_shader(GL_COMPUTE_SHADER, comp_source);
        move_particles = vgl::gl::create_program({ move_cs });
        auto spawn_cs = vgl::gl::create_shader(GL_COMPUTE_SHADER, spawn_src);
        spawn_particles = vgl::gl::create_program({ spawn_cs });
    };
    reload_shader();

    struct Particle {
        glm::vec4 pos;
        glm::vec4 vel;
        glm::vec4 accel;
        glm::vec3 color;
        float lifetime{ 0.0f };
    };

    struct Particle_system_config {
        glm::mat4 rot{};
        int update_count = 0;
        int offset = 0;
        float speed_scale = 0.1f;
        float lifetime_scale = 0.1f;
        float time = 0.0f;
    } system_config;

    auto particle_count = 1024 * 4 * block_length;
    auto size_p = particle_count * sizeof(Particle);
    auto p_ssbo = vgl::gl::create_buffer_fixed_size(size_p, GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, p_ssbo);

    auto vao = vgl::gl::create_vertex_array();

    float lower_freq = 0.0f;
    float upper_freq = fs / 2.0f;
    int particle_height = 16;
    system_config.update_count = particle_height * half_block;
    auto config_ssbo = vgl::gl::create_buffer(system_config, GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);
    const auto config_ptr = glMapNamedBufferRange(config_ssbo, 0, sizeof(system_config),
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, config_ssbo);

    std::vector<Particle> particles(system_config.update_count);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SPRITE);

    using high_res_clock = std::chrono::high_resolution_clock;
    auto current = high_res_clock::now();
    auto previous = high_res_clock::now();
    float dt = 0.0f;
    float acc = 0.0f;
    float update_time = 100.0f;

    impl::Recorder rec(fs, block_length);
    float point_size = 1.0f;
    constexpr auto twopi = 2.0f * glm::pi<float>();
    auto block_f = static_cast<float>(block_length);
    std::vector<float> fft_mag(block_length, 0.0f);
    auto fft_buffer = vgl::gl::create_buffer(fft_mag, GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, fft_buffer);
    auto fft_ptr = glMapNamedBufferRange(fft_buffer, 0, std::size(fft_mag) * sizeof(float),
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    while (!window.should_close()) {
        gui.start_frame();
        dt = std::chrono::duration<float>(current - previous).count();
        update_time += dt;
        previous = current;
        current = high_res_clock::now();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (ImGui::Begin("Vis")) {
            ImGui::DragFloat("Lower frequency", &lower_freq, 1.0f, 0.0f, upper_freq);
            ImGui::DragFloat("Upper frequency", &upper_freq, 1.0f, lower_freq, fs / 2.0f);
            ImGui::DragFloat("Point size", &point_size, 0.05f);
            ImGui::DragFloat("Speed scale", &system_config.speed_scale, 0.01f);
            ImGui::DragFloat("Lifetime scale", &system_config.lifetime_scale, 0.01f);
            ImGui::PlotLines("##wave", rec.data.data(), rec.data.size(), 0, "wave plot",
                -1, 1, ImVec2(500, 350), sizeof(float));
            ImGui::PlotHistogram("##FFT_mine", fft_mag.data(), fft_mag.size(), 0, "FFT plot",
                -1, 1, ImVec2(500, 350), sizeof(float));
        }
        ImGui::End();

        system_config.time += dt;
        system_config.rot = glm::mat4(glm::cos(acc), 0, -glm::sin(acc), 0, 0, 1.0f, 0, 0, glm::sin(acc), 0, glm::cos(acc), 0, 0, 0, 0, 1);
        acc += dt * 0.1f * glm::pi<float>();
        std::memcpy(config_ptr, &system_config, sizeof(system_config));

        if (rec.receive()) {
            std::lock_guard<std::mutex> lock(impl::audio_data_mutex);
            auto min_max_wave = std::minmax_element(rec.data.begin(), rec.data.end());
            auto max_abs = glm::max(glm::abs(*min_max_wave.first), glm::abs(*min_max_wave.second));
            if (max_abs > 0.0f) {
                for (int t = 0; t < rec.data.size(); ++t) {
                    rec.data.at(t) *= hann_function(t, block_length);
                }
                for (auto m = 0; m < block_length; ++m) {
                    float real = 0.0f;
                    float img = 0.0f;
                    auto frequency = twopi * m / block_f;
                    for (auto n = 0; n < block_length; ++n) {
                        auto data = rec.data.at(n);
                        real += data * glm::cos(frequency * n);
                        img += data * -1.0f * glm::sin(frequency * n);
                    }
                    auto ampl = glm::sqrt(real * real + img * img);
                    fft_mag.at(m) = 2.0f * ampl;
                }
                auto max_fft = *std::max_element(fft_mag.begin(), fft_mag.end());
                if (max_fft > 0.0f) {
                    for (auto& f : fft_mag) {
                        //f /= max_fft;
                    }
                    std::memcpy(fft_ptr, fft_mag.data(), std::size(fft_mag) * sizeof(float));
                    if (system_config.offset + system_config.update_count >= particle_count) {
                        system_config.offset = 0;
                    }

                    glUseProgram(spawn_particles);
                    std::array<int, 3> wgs;
                    glGetProgramiv(spawn_particles, GL_COMPUTE_WORK_GROUP_SIZE, wgs.data());
                    glDispatchCompute(glm::ceil(system_config.update_count / static_cast<float>(wgs.at(0))), 1, 1);

                    system_config.offset += system_config.update_count;
                }
            }
        }
        else {
            std::fill(fft_mag.begin(), fft_mag.end(), 0.0f);
            std::memcpy(fft_ptr, fft_mag.data(), std::size(fft_mag) * sizeof(float));
        }

        vgl::gl::update_uniform(render_particles, 0, point_size);

        glUseProgram(move_particles);
        std::array<int, 3> wgs;
        glGetProgramiv(move_particles, GL_COMPUTE_WORK_GROUP_SIZE, wgs.data());
        vgl::gl::update_uniform(move_particles, 0, dt);
        glDispatchCompute(glm::ceil(size_p / static_cast<float>(wgs.at(0))), 1, 1);
        glUseProgram(render_particles);
        glBindVertexArray(vao);
        glDrawArraysInstancedBaseInstance(GL_POINTS, 0, 1, particle_count, 0);
        
        gui.render();

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
                reload_shader();
            }
            vgl::gl::update_full_buffer(cam_ssbo, cam.get_cam_data());
        }
        if (!ImGui::IsAnyItemHovered() && !ImGui::IsAnyWindowHovered() && !ImGui::IsAnyItemFocused()
            && !ImGui::IsAnyItemActive()) {
            if (window.mouse[GLFW_MOUSE_BUTTON_LEFT]) {
                glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else {
                glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }

        window.swap_buffers();
        window.poll_events();
    }
    glUnmapNamedBuffer(p_ssbo);
}