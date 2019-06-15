#include "vgl/file/file.hpp"
#include "vgl/file/image_file.hpp"
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include "vgl/gpu_api/gl/texture.hpp"
#include "vgl/gpu_api/gl/framebuffer.hpp"
#include "vgl/gpu_api/gl/buffer.hpp"
#include "vgl/control/window.hpp"
#include "vgl/control/gui.hpp"
#include <glsp/glsp.hpp>
#include <random>

// enable optimus!
extern "C" {
    _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
}

float gen_random_float(const float lower = 0.0f, const float upper = 1.0f) {
    std::random_device rd;
    std::mt19937 eng(rd());
    const std::uniform_real_distribution<float> uni(lower, upper);
    return uni(eng);
}

int gen_random_int(const int lower = 0, const int upper = 255) {
    std::random_device rd;
    std::mt19937 eng(rd());
    const std::uniform_int_distribution<int> uni(lower, upper);
    return uni(eng);
}

int main() {
    auto w_res = glm::ivec2(1600, 900);
    vgl::Window window(w_res.x, w_res.y, "Hello");
    window.enable_gl();
    vgl::ui::Gui gui(window);
    glViewport(0, 0, w_res.x, w_res.y);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto vertex_shader_source = glsp::preprocess_file(vgl::file::shaders_path / "minimal/texture.vert").contents;
    auto frag_shader_source = glsp::preprocess_file(vgl::file::shaders_path / "cv/filters/rot13.frag").contents;
    auto vertex_shader = vgl::gl::create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto frag_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, frag_shader_source);

    const auto program = vgl::gl::create_program({vertex_shader, frag_shader});

    vgl::gl::gltexture tex_id, temp_tex;
    vgl::Image_desc tex_info;

    vgl::gl::glframebuffer temp_fbo;

    auto load_texture = [&]() {
        auto path = vgl::file::open_file_dialog(vgl::file::resources_path / "images");
        if (path) {
            auto image = vgl::file::load_image(path.value(), 4, true);
            tex_info = vgl::img::get_image_desc(image);
            tex_id = vgl::gl::create_texture(GL_TEXTURE_2D);
            glTextureParameteri(tex_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(tex_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(tex_id, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
            glTextureParameteri(tex_id, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

            auto internal_format = vgl::gl::derive_internal_format(image);
            glTextureStorage2D(tex_id, 1, internal_format, tex_info.width, tex_info.height);
            vgl::gl::set_texture_data_2d(tex_id, image);
            temp_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
            glTextureParameteri(temp_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(temp_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(temp_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
            glTextureParameteri(temp_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
            glTextureStorage2D(temp_tex, 1, GL_RGBA8, tex_info.width, tex_info.height);
            temp_fbo = vgl::gl::create_framebuffer();
            glNamedFramebufferTexture(temp_fbo, GL_COLOR_ATTACHMENT0, temp_tex, 0);
            vgl::gl::attach_draw_buffers(temp_fbo, { GL_COLOR_ATTACHMENT0 });
            vgl::gl::check_framebuffer(temp_fbo);
        }
    };

    auto screen_vao = vgl::gl::create_vertex_array();
    vgl::gl::glbuffer one_time_pad;
    int rot_value = 0;
    int type = 0;
    int max_value = 360;
    int xor_value = gen_random_int();
    glm::mat3 hill_mat(0.0f);
    hill_mat[0][0] = gen_random_float();
    hill_mat[0][1] = gen_random_float();
    hill_mat[0][2] = gen_random_float();
    hill_mat[1][0] = gen_random_float();
    hill_mat[1][1] = gen_random_float();
    hill_mat[1][2] = gen_random_float();
    hill_mat[2][0] = gen_random_float();
    hill_mat[2][1] = gen_random_float();
    hill_mat[2][2] = gen_random_float();
    while (!window.should_close()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gui.start_frame();
        if (ImGui::Begin("Filter settings")) {
            if (ImGui::Button("Load image")) {
                load_texture();
            }
            ImGui::Combo("Rot type", &type, "hsv\0rgb\0hill\0xor\0bitreverse\0onetimepad\0reverse_onetimepad\0");
            if (type == 0) {
                max_value = 360;
            }
            else if (type == 1) {
                max_value = 256;
                rot_value = glm::min(rot_value, max_value);
            }
            if (type <= 1) {
                ImGui::SliderInt("Rot x", &rot_value, 0, max_value);
            }
            else if (type == 2) {
                if (ImGui::Button("New random hill matrix")) {
                    hill_mat[0][0] = gen_random_float();
                    hill_mat[0][1] = gen_random_float();
                    hill_mat[0][2] = gen_random_float();
                    hill_mat[1][0] = gen_random_float();
                    hill_mat[1][1] = gen_random_float();
                    hill_mat[1][2] = gen_random_float();
                    hill_mat[2][0] = gen_random_float();
                    hill_mat[2][1] = gen_random_float();
                    hill_mat[2][2] = gen_random_float();
                }
            }
            else if (type == 3) {
                if (ImGui::Button("New random xor value")) {
                    xor_value = gen_random_int();
                }
            }
            else if (type == 5) {
                if (ImGui::Button("New one time pad")) {
                    std::vector<float> one_time_pad_data(tex_info.width * tex_info.height * tex_info.channels);
                    std::random_device rd;
                    std::mt19937 eng(rd());
                    const std::uniform_real_distribution<float> uni(0.0f, 1.0f);
                    std::generate(one_time_pad_data.begin(), one_time_pad_data.end(), [&uni, &eng]() {
                        return uni(eng);
                        });
                    one_time_pad = vgl::gl::create_buffer(one_time_pad_data);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, one_time_pad);
                }
            }
            vgl::gl::update_uniform(program, 0, rot_value);
            vgl::gl::update_uniform(program, 1, type);
            vgl::gl::update_uniform(program, 2, hill_mat);
            vgl::gl::update_uniform(program, 3, xor_value);
            if (tex_id != 0) {
                if (ImGui::Button("Save result")) {
                    auto file_path = vgl::file::save_file_dialog(vgl::file::resources_path / "images/", "png,jpg");
                    if (file_path) {
                        glBindFramebuffer(GL_FRAMEBUFFER, temp_fbo);
                        glViewport(0, 0, tex_info.width, tex_info.height);
                        glUseProgram(program);
                        glBindTextureUnit(0, tex_id);
                        glBindVertexArray(screen_vao);
                        glDrawArrays(GL_TRIANGLES, 0, 3);
                        vgl::Image_desc desc{ tex_info.width, tex_info.height, tex_info.channels };
                        vgl::Image_uc img(desc);

                        glGetTextureImage(temp_tex, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data.size() * sizeof(unsigned char), img.data.data());
                        if (vgl::file::save_image(img, file_path.value(), true)) {

                        }
                        glBindFramebuffer(GL_FRAMEBUFFER, 0);
                        auto win_size = window.size();
                        glViewport(0, 0, win_size.x, win_size.y);
                    }
                }
            }
        }
        ImGui::End();
        glUseProgram(program);
        glBindTextureUnit(0, tex_id);
        glBindVertexArray(screen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        gui.render();
        window.swap_buffers();
        window.poll_events();
    }
}
