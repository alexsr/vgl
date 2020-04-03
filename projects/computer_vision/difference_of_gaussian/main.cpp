#include <Eigen/Core>
#include "vgl/file/file.hpp"
#include "vgl/file/image_file.hpp"
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include "vgl/gpu_api/gl/texture.hpp"
#include "vgl/gpu_api/gl/framebuffer.hpp"
#include "vgl/core/window.hpp"
#include "vgl/core/gui.hpp"
#include <glsp/glsp.hpp>

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

    auto vertex_shader_source = glsp::preprocess_file(vgl::file::shaders_path / "minimal/texture.vert").contents;
    auto vertex_shader = vgl::gl::create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto monochrome_source = glsp::preprocess_file(vgl::file::shaders_path / "cv/filters/monochrome.frag").contents;
    auto monochrome_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, monochrome_source);
    auto dog_first_pass_source = glsp::preprocess_file(vgl::file::shaders_path
        / "cv/blob_detection/dog_first_pass.frag").contents;
    auto dog_first_pass_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, dog_first_pass_source);
    auto dog_second_pass_source = glsp::preprocess_file(vgl::file::shaders_path
        / "cv/blob_detection/dog_second_pass.frag").contents;
    auto dog_second_pass_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, dog_second_pass_source);

    const auto monochrome = vgl::gl::create_program({vertex_shader, monochrome_shader});
    const auto dog_first_pass = vgl::gl::create_program({vertex_shader, dog_first_pass_shader});
    const auto dog_second_pass = vgl::gl::create_program({vertex_shader, dog_second_pass_shader});

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
    auto ptr = std::unique_ptr<stbi_uc>(stbi_load(image_path.c_str(), &image_size.x(), &image_size.y(),
                                                  &image_channels, image_channels));

    glTextureStorage2D(tex_id, 1, GL_RGBA8, image_size.x(), image_size.y());
    glTextureSubImage2D(tex_id, 0, 0, 0, image_size.x(), image_size.y(), GL_RGBA, GL_UNSIGNED_BYTE, ptr.get());

    auto monochrome_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(monochrome_tex, 1, GL_R32F, image_size.x(), image_size.y());

    auto monochrome_fb = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(monochrome_fb, GL_COLOR_ATTACHMENT0, monochrome_tex, 0);
    vgl::gl::attach_draw_buffers(monochrome_fb, { GL_COLOR_ATTACHMENT0 });
    vgl::gl::check_framebuffer(monochrome_fb);

    auto gauss_x_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(gauss_x_tex, 1, GL_RG32F, image_size.x(), image_size.y());

    auto framebuffer_gauss = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(framebuffer_gauss, GL_COLOR_ATTACHMENT0, gauss_x_tex, 0);
    vgl::gl::attach_draw_buffers(framebuffer_gauss, { GL_COLOR_ATTACHMENT0 });
    vgl::gl::check_framebuffer(framebuffer_gauss);

    auto screen_vao = vgl::gl::create_vertex_array();

    int kernel_small = 1;
    int kernel_large = 3;

    while (!window.should_close()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gui.start_frame();
        if (ImGui::Begin("Kernel size")) {
            ImGui::SliderInt("Kernel small", &kernel_small, 0, kernel_large - 1);
            ImGui::SliderInt("Kernel large", &kernel_large, kernel_small + 1, 100);
            glProgramUniform1i(dog_first_pass, 0, kernel_small);
            glProgramUniform1i(dog_first_pass, 1, kernel_large);
            glProgramUniform1i(dog_second_pass, 0, kernel_small);
            glProgramUniform1i(dog_second_pass, 1, kernel_large);
        }
        ImGui::End();
        glUseProgram(monochrome);
        glBindTextureUnit(0, tex_id);
        glViewport(0, 0, image_size.x(), image_size.y());
        glBindFramebuffer(GL_FRAMEBUFFER, monochrome_fb);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(screen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(dog_first_pass);
        glBindTextureUnit(0, monochrome_tex);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_gauss);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(dog_second_pass);
        glBindTextureUnit(0, gauss_x_tex);
        glViewport(0, 0, w_res.x(), w_res.y());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        gui.render();
        window.swap_buffers();
        window.poll_events();
    }
}
