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

    auto vertex_shader_source = glsp::preprocess_file(vgl::file::shaders_path / "minimal/texture.vert").contents;
    auto vertex_shader = vgl::gl::create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto monochrome_shader_source = glsp::preprocess_file(vgl::file::shaders_path / "cv/filters/monochrome.frag").contents;
    auto monochrome_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, monochrome_shader_source);
    auto moravec_min_E_source = glsp::preprocess_file(vgl::file::shaders_path / "cv/corner_detection/moravec_min_E.frag").contents;
    auto moravec_min_E_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, moravec_min_E_source);
    auto moravec_draw_corner_source = glsp::preprocess_file(vgl::file::shaders_path / "cv/corner_detection/non_max_suppression_overlay.frag").contents;
    auto moravec_draw_corner_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, moravec_draw_corner_source);

    const auto monochrome = vgl::gl::create_program({vertex_shader, monochrome_shader});
    const auto moravec_min_E = vgl::gl::create_program({vertex_shader, moravec_min_E_shader});
    const auto moravec_draw_corner = vgl::gl::create_program({vertex_shader, moravec_draw_corner_shader});

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

    auto corner_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(corner_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(corner_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(corner_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(corner_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(corner_tex, 1, GL_R32F, image_size.x, image_size.y);

    auto monochrome_fb = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(monochrome_fb, GL_COLOR_ATTACHMENT0, monochrome_tex, 0);
    vgl::gl::attach_draw_buffers(monochrome_fb, { GL_COLOR_ATTACHMENT0 });
    vgl::gl::check_framebuffer(monochrome_fb);

    GLuint corner_fb = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(corner_fb, GL_COLOR_ATTACHMENT0, corner_tex, 0);
    vgl::gl::attach_draw_buffers(corner_fb, { GL_COLOR_ATTACHMENT0 });
    vgl::gl::check_framebuffer(corner_fb);

    auto screen_vao = vgl::gl::create_vertex_array();

    float min_threshold = 0.5f;
    int kernel_size = 3;

    while (!window.should_close()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gui.start_frame();
        if (ImGui::Begin("Moravec")) {
            ImGui::DragFloat("Threshold", &min_threshold, 0.01f);
            ImGui::SliderInt("Kernel Size", &kernel_size, 1, 100);
            glProgramUniform1f(moravec_draw_corner, 1, min_threshold);
            glProgramUniform1i(moravec_draw_corner, 0, kernel_size);
            glProgramUniform1i(moravec_min_E, 0, kernel_size);
        }
        ImGui::End();
        glUseProgram(monochrome);
        glBindTextureUnit(0, tex_id);
        glBindVertexArray(screen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glViewport(0, 0, image_size.x, image_size.y);
        glBindFramebuffer(GL_FRAMEBUFFER, monochrome_fb);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(moravec_min_E);
        glBindTextureUnit(0, monochrome_tex);
        glBindFramebuffer(GL_FRAMEBUFFER, corner_fb);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(moravec_draw_corner);
        glBindTextureUnit(0, corner_tex);
        glViewport(0, 0, w_res.x, w_res.y);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        gui.render();
        window.swap_buffers();
        window.poll_events();
    }
}
