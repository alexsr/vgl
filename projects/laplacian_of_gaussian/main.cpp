#include "GLFW/glfw3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "glm/glm.hpp"
#include "vgl/file/file.hpp"
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/gpu_api/gl/buffer.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include "vgl/gpu_api/gl/texture.hpp"
#include "vgl/gpu_api/gl/fbo.hpp"
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"

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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto vertex_shader = vgl::gl::create_shader_spirv(GL_VERTEX_SHADER, vgl::shaders_path / "minimal/texture.vert");
    auto monochrome_shader = vgl::gl::create_shader_spirv(GL_FRAGMENT_SHADER, vgl::shaders_path / "filters/monochrome.frag");
    auto gauss_x_shader = vgl::gl::create_shader_spirv(GL_FRAGMENT_SHADER, vgl::shaders_path / "filters/gauss_x.frag");
    auto gauss_y_shader = vgl::gl::create_shader_spirv(GL_FRAGMENT_SHADER, vgl::shaders_path / "filters/gauss_y.frag");
    auto laplacian_shader = vgl::gl::create_shader_spirv(GL_FRAGMENT_SHADER, vgl::shaders_path / "blob_detection/laplacian_normalized.frag");

    const auto monochrome = vgl::gl::create_program(vertex_shader, monochrome_shader);
    const auto gauss_x = vgl::gl::create_program(vertex_shader, gauss_x_shader);
    const auto gauss_y = vgl::gl::create_program(vertex_shader, gauss_y_shader);
    const auto laplacian = vgl::gl::create_program(vertex_shader, laplacian_shader);

    vgl::gl::delete_shaders(vertex_shader, monochrome_shader, gauss_x_shader, gauss_y_shader, laplacian_shader);

    GLuint tex_id = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(tex_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex_id, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(tex_id, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    glm::ivec2 image_size{};
    auto image_path = (vgl::resources_path / "images/clifton-house-project.jpg").string();
    stbi_info(image_path.c_str(), &image_size.x, &image_size.y, nullptr);
    stbi_set_flip_vertically_on_load(1);
    auto image_channels = 4;
    auto ptr = std::unique_ptr<stbi_uc>(stbi_load(image_path.c_str(), &image_size.x, &image_size.y,
                                                  &image_channels, image_channels));

    glTextureStorage2D(tex_id, 1, GL_RGBA8, image_size.x, image_size.y);
    glTextureSubImage2D(tex_id, 0, 0, 0, image_size.x, image_size.y, GL_RGBA, GL_UNSIGNED_BYTE, ptr.get());

    GLuint gauss_x_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(gauss_x_tex, 1, GL_RGBA32F, image_size.x, image_size.y);

    GLuint gauss_y_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(gauss_y_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_y_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_y_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(gauss_y_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(gauss_y_tex, 1, GL_RGBA32F, image_size.x, image_size.y);

    GLuint framebuffer_gauss_x = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(framebuffer_gauss_x, GL_COLOR_ATTACHMENT0, gauss_x_tex, 0);
    vgl::gl::attach_drawbuffers(framebuffer_gauss_x, GL_COLOR_ATTACHMENT0);
    vgl::gl::check_framebuffer(framebuffer_gauss_x);

    GLuint framebuffer_gauss_y = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(framebuffer_gauss_y, GL_COLOR_ATTACHMENT0, gauss_y_tex, 0);
    vgl::gl::attach_drawbuffers(framebuffer_gauss_y, GL_COLOR_ATTACHMENT0);
    vgl::gl::check_framebuffer(framebuffer_gauss_y);

    auto screen_vao = vgl::gl::create_vertex_array();

    int kernel_size = 0;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (ImGui::Begin("Kernel size")) {
            ImGui::SliderInt("Kernel Size", &kernel_size, 0, 100);
            glProgramUniform1i(gauss_x, 0, kernel_size);
            glProgramUniform1i(gauss_y, 0, kernel_size);
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
        glUseProgram(gauss_x);
        glBindTextureUnit(0, gauss_y_tex);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_gauss_x);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(gauss_y);
        glBindTextureUnit(0, gauss_x_tex);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_gauss_y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(laplacian);
        glBindTextureUnit(0, gauss_y_tex);
        glViewport(0, 0, w_res.x, w_res.y);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}
