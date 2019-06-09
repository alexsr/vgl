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
    auto vertex_shader = vgl::gl::create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto monochrome_shader_source = glsp::preprocess_file((vgl::file::shaders_path / "cv/filters/monochrome.frag").string()).contents;
    auto monochrome_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, monochrome_shader_source);
    auto gauss_source = glsp::preprocess_file((vgl::file::shaders_path / "cv/filters/gauss.frag").string()).contents;
    auto gauss_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, gauss_source);
    auto laplacian_source = glsp::preprocess_file((vgl::file::shaders_path / "cv/blob_detection/laplacian_normalized.frag").string()).contents;
    auto laplacian_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, laplacian_source);

    const auto monochrome = vgl::gl::create_program({vertex_shader, monochrome_shader});
    const auto gauss = vgl::gl::create_program({vertex_shader, gauss_shader});
    const auto laplacian = vgl::gl::create_program({vertex_shader, laplacian_shader});

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

    auto gauss_x_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(gauss_x_tex, 1, GL_RGBA32F, image_size.x, image_size.y);

    auto gauss_y_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(gauss_y_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_y_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_y_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(gauss_y_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(gauss_y_tex, 1, GL_RGBA32F, image_size.x, image_size.y);

    auto framebuffer_gauss_x = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(framebuffer_gauss_x, GL_COLOR_ATTACHMENT0, gauss_x_tex, 0);
    vgl::gl::attach_draw_buffers(framebuffer_gauss_x, { GL_COLOR_ATTACHMENT0 });
    vgl::gl::check_framebuffer(framebuffer_gauss_x);

    auto framebuffer_gauss_y = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(framebuffer_gauss_y, GL_COLOR_ATTACHMENT0, gauss_y_tex, 0);
    vgl::gl::attach_draw_buffers(framebuffer_gauss_y, { GL_COLOR_ATTACHMENT0 });
    vgl::gl::check_framebuffer(framebuffer_gauss_y);

    auto screen_vao = vgl::gl::create_vertex_array();

    int kernel_size = 0;

    while (!window.should_close()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gui.start_frame();
        if (ImGui::Begin("Kernel size")) {
            ImGui::SliderInt("Kernel Size", &kernel_size, 0, 100);
            glProgramUniform1i(gauss, 0, kernel_size);
            glProgramUniform1i(laplacian, 0, kernel_size);
        }
        ImGui::End();
        glUseProgram(monochrome);
        glBindTextureUnit(0, tex_id);
        glViewport(0, 0, image_size.x, image_size.y);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_gauss_y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(screen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(gauss);
        vgl::gl::update_uniform(gauss, 1, 0);
        glBindTextureUnit(0, gauss_y_tex);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_gauss_x);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        vgl::gl::update_uniform(gauss, 1, 1);
        glBindTextureUnit(0, gauss_x_tex);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_gauss_y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(laplacian);
        glBindTextureUnit(0, gauss_y_tex);
        glViewport(0, 0, w_res.x, w_res.y);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        gui.render();
        window.swap_buffers();
        window.poll_events();
    }
}
