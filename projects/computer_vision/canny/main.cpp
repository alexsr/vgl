#include <Eigen/Core>
#include "vgl/file/file.hpp"
#include "vgl/file/image_file.hpp"
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include "vgl/gpu_api/gl/texture.hpp"
#include "vgl/gpu_api/gl/framebuffer.hpp"
#include "vgl/core/window.hpp"
#include "vgl/core/gui.hpp"

// enable optimus!
extern "C" {
_declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
}

int main() {
    auto w_res = Eigen::Vector2i(1600, 900);
    vgl::Window window(w_res.x(), w_res.y(), "Hello");
    window.enable_gl();
    vgl::ui::Gui gui(window);
    glViewport(0, 0, w_res.x(), w_res.y());

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto vertex_shader =
        vgl::gl::create_shader_from_file(GL_VERTEX_SHADER, vgl::file::shaders_path / "minimal/texture.vert");
    auto gauss_shader =
        vgl::gl::create_shader_from_file(GL_FRAGMENT_SHADER, vgl::file::shaders_path / "cv/filters/gauss.frag");
    const auto gauss = vgl::gl::create_program({vertex_shader, gauss_shader});

    auto monochrome_shader =
        vgl::gl::create_shader_from_file(GL_FRAGMENT_SHADER, vgl::file::shaders_path / "cv/filters/monochrome.frag");
    auto sobel_first_shader =
        vgl::gl::create_shader_from_file(GL_FRAGMENT_SHADER, vgl::file::shaders_path / "cv/filters/sobel_xy.frag");
    auto sobel_second_shader = vgl::gl::create_shader_from_file(
        GL_FRAGMENT_SHADER, vgl::file::shaders_path / "cv/filters/sobel_xy_angle.frag");
    auto non_max_suppression_shader = vgl::gl::create_shader_from_file(
        GL_FRAGMENT_SHADER, vgl::file::shaders_path / "cv/edge_detection/canny_non_max_suppression.frag");
    auto double_threshold_shader = vgl::gl::create_shader_from_file(
        GL_FRAGMENT_SHADER, vgl::file::shaders_path / "cv/edge_detection/canny_double_threshold.frag");

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

    Eigen::Vector2i image_size{};
    auto image_path = (vgl::file::resources_path / "images/clifton-house-project.jpg").string();
    stbi_info(image_path.c_str(), &image_size.x(), &image_size.y(), nullptr);
    stbi_set_flip_vertically_on_load(1);
    auto image_channels = 4;
    auto ptr = std::unique_ptr<stbi_uc>(
        stbi_load(image_path.c_str(), &image_size.x(), &image_size.y(), &image_channels, image_channels));

    glTextureStorage2D(tex_id, 1, GL_RGBA8, image_size.x(), image_size.y());
    glTextureSubImage2D(tex_id, 0, 0, 0, image_size.x(), image_size.y(), GL_RGBA, GL_UNSIGNED_BYTE, ptr.get());

    auto monochrome_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(monochrome_tex, 1, GL_RGBA32F, image_size.x(), image_size.y());

    auto sobel_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(sobel_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(sobel_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(sobel_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(sobel_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(sobel_tex, 1, GL_RGBA32F, image_size.x(), image_size.y());

    auto monochrome_fb = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(monochrome_fb, GL_COLOR_ATTACHMENT0, monochrome_tex, 0);
    vgl::gl::attach_draw_buffers(monochrome_fb, {GL_COLOR_ATTACHMENT0});
    vgl::gl::check_framebuffer(monochrome_fb);

    auto sobel_fb = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(sobel_fb, GL_COLOR_ATTACHMENT0, sobel_tex, 0);
    vgl::gl::attach_draw_buffers(sobel_fb, {GL_COLOR_ATTACHMENT0});
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
            glProgramUniform1i(gauss, 0, kernel_size);
            glProgramUniform1i(non_max_suppression, 0, non_max_size);
            glProgramUniform1f(non_max_suppression, 1, non_max_threshold);
            glProgramUniform1f(double_threshold, 0, threshold_low);
            glProgramUniform1f(double_threshold, 1, threshold_high);
        }
        ImGui::End();
        glUseProgram(gauss);
        vgl::gl::update_uniform(gauss, 1, 0);
        glBindTextureUnit(0, tex_id);
        glViewport(0, 0, image_size.x(), image_size.y());
        glBindFramebuffer(GL_FRAMEBUFFER, monochrome_fb);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(screen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(gauss);
        vgl::gl::update_uniform(gauss, 1, 1);
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
        glViewport(0, 0, w_res.x(), w_res.y());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        gui.render();
        window.swap_buffers();
        window.poll_events();
    }
}
