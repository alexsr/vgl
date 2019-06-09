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
    auto frag_shader_source = glsp::preprocess_file((vgl::file::shaders_path / "cv/filters/rot13.frag").string()).contents;
    auto vertex_shader = vgl::gl::create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto frag_shader = vgl::gl::create_shader(GL_FRAGMENT_SHADER, frag_shader_source);

    const auto program = vgl::gl::create_program({vertex_shader, frag_shader});

    vgl::gl::gltexture tex_id;

    auto load_texture = [&]() {
        auto path = vgl::file::open_file_dialog(vgl::file::resources_path / "images");
        if (path) {
            auto tex_info = vgl::file::retrieve_image_desc(path.value(), 4);
            auto tex_data = vgl::file::load_image(path.value(), 4, true);
            tex_id = vgl::gl::create_texture(GL_TEXTURE_2D);
            glTextureParameteri(tex_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(tex_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(tex_id, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
            glTextureParameteri(tex_id, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
            glTextureStorage2D(tex_id, 1, tex_info.internal_format, tex_info.image_size.x, tex_info.image_size.y);
            vgl::gl::set_texture_data_2d(tex_id, tex_data);
        }
    };

    auto screen_vao = vgl::gl::create_vertex_array();

    int rot_value = 0;

    while (!window.should_close()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gui.start_frame();
        if (ImGui::Begin("Filter settings")) {
            if (ImGui::Button("Load image")) {
                load_texture();
            }
            ImGui::SliderInt("Rot x", &rot_value, 0, 360);
            vgl::gl::update_uniform(program, 0, rot_value);
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
