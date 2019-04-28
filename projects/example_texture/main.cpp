#include "GLFW/glfw3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "glm/glm.hpp"
#include "vgl/file/file.hpp"
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/gpu_api/gl/buffer.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include "vgl/gpu_api/gl/texture.hpp"

int main() {
    glfwInit();
    auto w_res = glm::ivec2(1600, 900);
    auto window = glfwCreateWindow(w_res.x, w_res.y, "Hello", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glViewport(0, 0, w_res.x, w_res.y);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto vertex_shader = vgl::gl::create_shader_spirv(GL_VERTEX_SHADER, vgl::shaders_path / "minimal/texture.vert");
    auto frag_shader = vgl::gl::create_shader_spirv(GL_FRAGMENT_SHADER, vgl::shaders_path / "minimal/texture.frag");

    const auto program = vgl::gl::create_program(vertex_shader, frag_shader);

    vgl::gl::delete_shaders(vertex_shader, frag_shader);

    GLuint tex_id = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(tex_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex_id, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTextureParameteri(tex_id, GL_TEXTURE_WRAP_S, GL_REPEAT);

    glm::ivec2 image_size{};
    auto image_path = (vgl::resources_path / "images/campus.jpg").string();
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

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(screen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}
