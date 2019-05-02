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
    auto dog_first_pass_shader = vgl::gl::create_shader_spirv(GL_FRAGMENT_SHADER, vgl::shaders_path / "blob_detection/dog_first_pass.frag");
    auto dog_second_pass_shader = vgl::gl::create_shader_spirv(GL_FRAGMENT_SHADER, vgl::shaders_path / "blob_detection/dog_second_pass.frag");

    const auto monochrome = vgl::gl::create_program(vertex_shader, monochrome_shader);
    const auto dog_first_pass = vgl::gl::create_program(vertex_shader, dog_first_pass_shader);
    const auto dog_second_pass = vgl::gl::create_program(vertex_shader, dog_second_pass_shader);

    vgl::gl::delete_shaders(vertex_shader, dog_first_pass_shader, dog_second_pass_shader, monochrome_shader);

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

    GLuint monochrome_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(monochrome_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(monochrome_tex, 1, GL_R32F, image_size.x, image_size.y);

    GLuint monochrome_fb = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(monochrome_fb, GL_COLOR_ATTACHMENT0, monochrome_tex, 0);
    vgl::gl::attach_drawbuffers(monochrome_fb, GL_COLOR_ATTACHMENT0);
    vgl::gl::check_framebuffer(monochrome_fb);

    GLuint gauss_x_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(gauss_x_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(gauss_x_tex, 1, GL_RG32F, image_size.x, image_size.y);

    /*GLuint gauss_y_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(gauss_y_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_y_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(gauss_y_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(gauss_y_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(gauss_y_tex, 1, GL_RG32F, image_size.x, image_size.y);*/

    GLuint framebuffer_gauss = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(framebuffer_gauss, GL_COLOR_ATTACHMENT0, gauss_x_tex, 0);
    //glNamedFramebufferTexture(framebuffer_gauss, GL_COLOR_ATTACHMENT1, gauss_y_tex, 0);
    vgl::gl::attach_drawbuffers(framebuffer_gauss, GL_COLOR_ATTACHMENT0);
    vgl::gl::check_framebuffer(framebuffer_gauss);

    auto screen_vao = vgl::gl::create_vertex_array();

    int kernel_small = 1;
    int kernel_large = 3;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
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
        glViewport(0, 0, image_size.x, image_size.y);
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
