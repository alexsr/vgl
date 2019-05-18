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
    auto frag_shader_source = glsp::preprocess_file((vgl::file::shaders_path / "filters/texture_filters.frag").string()).contents;
    auto vertex_shader = vgl::gl::create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto frag_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, frag_shader_source);

    const auto program = vgl::gl::create_program({vertex_shader, frag_shader});

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
    glBindTextureUnit(0, tex_id);
    glUseProgram(program);

    auto screen_vao = vgl::gl::create_vertex_array();

    int kernel_size = 0;
    int filter = 0;
    float strength = 0.0f;

    while (!window.should_close()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gui.start_frame();
        if (ImGui::Begin("Kernel size")) {
            ImGui::SliderInt("Filter", &filter, 0, 100);
            ImGui::SliderInt("Kernel Size", &kernel_size, 0, 100);
            ImGui::DragFloat("Strength", &strength, 0.01f, 0.0f, 100.0f);
            glUniform1i(0, kernel_size);
            glUniform1i(1, filter);
            glUniform1f(2, strength);
        }
        ImGui::End();
        glBindVertexArray(screen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        gui.render();
        window.swap_buffers();
        window.poll_events();
    }
}
