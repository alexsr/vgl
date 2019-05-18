#include "glm/glm.hpp"
#include "vgl/file/file.hpp"
#include "vgl/file/image_file.hpp"
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include "vgl/gpu_api/gl/texture.hpp"
#include "vgl/gpu_api/gl/framebuffer.hpp"
#include "vgl/control/window.hpp"
#include "vgl/control/gui.hpp"
#include <glsp/glsp.hpp>

// enable optimus!
extern "C" {
    _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
}

int main() {
    auto w_res = glm::ivec2(1600, 900);
    vgl::Window window(w_res.x, w_res.y, "Hello");
    window.enable_gl();
    vgl::ui::Gui gui(window);
    glViewport(0, 0, w_res.x, w_res.y);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto vertex_shader_source = glsp::preprocess_file((vgl::file::shaders_path / "minimal/texture.vert").string()).contents;
    auto gauss_x_source = glsp::preprocess_file((vgl::file::shaders_path / "filters/gauss_x.frag").string()).contents;
    auto gauss_y_source = glsp::preprocess_file((vgl::file::shaders_path / "filters/gauss_y.frag").string()).contents;
    auto vertex_shader = vgl::gl::create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto gauss_x_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, gauss_x_source);
    auto gauss_y_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, gauss_y_source);
    const auto gauss_x = vgl::gl::create_program({ vertex_shader, gauss_x_shader });
    const auto gauss_y = vgl::gl::create_program({ vertex_shader, gauss_y_shader });

    auto monochrome_source = glsp::preprocess_file((vgl::file::shaders_path / "filters/monochrome.frag").string()).contents;
    auto monochrome_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, monochrome_source);

    auto sobel_first_source = glsp::preprocess_file((vgl::file::shaders_path / "filters/sobel_xy_first_pass.frag").string()).contents;
    auto sobel_first_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, sobel_first_source);

    auto sobel_second_source = glsp::preprocess_file((vgl::file::shaders_path / "filters/sobel_xy_angle.frag").string()).contents;
    auto sobel_second_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, sobel_second_source);

    auto non_max_suppression_source = glsp::preprocess_file((vgl::file::shaders_path
        / "edge_detection/canny_non_max_suppression.frag").string()).contents;
    auto non_max_suppression_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, non_max_suppression_source);

    auto double_threshold_source = glsp::preprocess_file((vgl::file::shaders_path
        / "edge_detection/canny_double_threshold.frag").string()).contents;
    auto double_threshold_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, double_threshold_source);

    const auto monochrome = vgl::gl::create_program({vertex_shader, monochrome_shader});
    const auto sobel_xy_first = vgl::gl::create_program({vertex_shader, sobel_first_shader});
    const auto sobel_xy_second = vgl::gl::create_program({vertex_shader, sobel_second_shader});
    const auto double_threshold = vgl::gl::create_program({vertex_shader, double_threshold_shader});
    const auto non_max_suppression = vgl::gl::create_program({vertex_shader, non_max_suppression_shader});

    auto tex_id = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(tex_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex_id, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(tex_id, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    glm::ivec2 image_size{};
    auto image_path = (vgl::file::resources_path / "images/clifton-house-project.jpg").string();
    stbi_info(image_path.c_str(), &image_size.x, &image_size.y, nullptr);
    stbi_set_flip_vertically_on_load(1);
    auto image_channels = 4;
    auto ptr = std::unique_ptr<stbi_uc>(stbi_load(image_path.c_str(), &image_size.x, &image_size.y,
                                                  &image_channels, image_channels));

    glTextureStorage2D(tex_id, 1, GL_RGBA8, image_size.x, image_size.y);
    glTextureSubImage2D(tex_id, 0, 0, 0, image_size.x, image_size.y, GL_RGBA, GL_UNSIGNED_BYTE, ptr.get());

    auto monochrome_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(monochrome_tex, 1, GL_RGBA32F, image_size.x, image_size.y);

    auto sobel_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(sobel_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(sobel_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(sobel_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(sobel_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(sobel_tex, 1, GL_RGBA32F, image_size.x, image_size.y);

    auto monochrome_fb = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(monochrome_fb, GL_COLOR_ATTACHMENT0, monochrome_tex, 0);
    vgl::gl::attach_draw_buffers(monochrome_fb, { GL_COLOR_ATTACHMENT0 });
    vgl::gl::check_framebuffer(monochrome_fb);

    auto sobel_fb = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(sobel_fb, GL_COLOR_ATTACHMENT0, sobel_tex, 0);
    vgl::gl::attach_draw_buffers(sobel_fb, { GL_COLOR_ATTACHMENT0 });
    vgl::gl::check_framebuffer(sobel_fb);

    auto screen_vao = vgl::gl::create_vertex_array();

    float non_max_threshold = 0.04f;
    float threshold_low = 0.04f;
    float threshold_high = 0.1f;
    int kernel_size = 3;
    int non_max_size = 3;

    while (!window.should_close()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gui.start_frame();
        if (ImGui::Begin("Canny")) {
            ImGui::SliderInt("Gauss kernel size", &kernel_size, 1, 100);
            ImGui::SliderInt("Kernel Size Non Max Suppression", &non_max_size, 1, 100);
            ImGui::DragFloat("Threshold non max", &non_max_threshold, 0.01f);
            ImGui::DragFloat("Threshold low", &threshold_low, 0.01f, 0.0f, threshold_high);
            ImGui::DragFloat("Threshold high", &threshold_high, 0.01f, threshold_low, 100.0f);
            glProgramUniform1i(gauss_x, 0, kernel_size);
            glProgramUniform1i(gauss_y, 0, kernel_size);
            glProgramUniform1i(non_max_suppression, 0, non_max_size);
            glProgramUniform1f(non_max_suppression, 1, non_max_threshold);
            glProgramUniform1f(double_threshold, 0, threshold_low);
            glProgramUniform1f(double_threshold, 1, threshold_high);
        }
        ImGui::End();
        glUseProgram(gauss_x);
        glBindTextureUnit(0, tex_id);
        glViewport(0, 0, image_size.x, image_size.y);
        glBindFramebuffer(GL_FRAMEBUFFER, monochrome_fb);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(screen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(gauss_y);
        glBindTextureUnit(0, monochrome_tex);
        glBindFramebuffer(GL_FRAMEBUFFER, sobel_fb);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(monochrome);
        glBindTextureUnit(0, sobel_tex);
        glBindFramebuffer(GL_FRAMEBUFFER, monochrome_fb);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(sobel_xy_first);
        glBindTextureUnit(0, monochrome_tex);
        glBindFramebuffer(GL_FRAMEBUFFER, sobel_fb);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(sobel_xy_second);
        glBindTextureUnit(0, sobel_tex);
        glBindFramebuffer(GL_FRAMEBUFFER, monochrome_fb);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(non_max_suppression);
        glBindTextureUnit(0, monochrome_tex);
        glBindFramebuffer(GL_FRAMEBUFFER, sobel_fb);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(double_threshold);
        glBindTextureUnit(0, sobel_tex);
        w_res = window.size();
        glViewport(0, 0, w_res.x, w_res.y);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        gui.render();
        window.swap_buffers();
        window.poll_events();
    }
}
