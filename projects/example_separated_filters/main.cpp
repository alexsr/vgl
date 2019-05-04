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
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto vertex_shader = vgl::gl::create_shader_spirv(GL_VERTEX_SHADER, vgl::shaders_path / "minimal/texture.vert");
    auto first_pass_shader = vgl::gl::create_shader_spirv(
        GL_FRAGMENT_SHADER, vgl::shaders_path / "filters/separated_first_pass.frag");
    auto second_pass_shader = vgl::gl::create_shader_spirv(
        GL_FRAGMENT_SHADER, vgl::shaders_path / "filters/separated_second_pass.frag");

    const auto first_pass = vgl::gl::create_program({vertex_shader, first_pass_shader});
    const auto second_pass = vgl::gl::create_program({vertex_shader, second_pass_shader});

    vgl::gl::delete_shaders({vertex_shader, first_pass_shader, second_pass_shader});

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
    glBindTextureUnit(0, tex_id);

    GLuint intermediate_tex = vgl::gl::create_texture(GL_TEXTURE_2D);
    glTextureParameteri(intermediate_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(intermediate_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(intermediate_tex, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(intermediate_tex, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureStorage2D(intermediate_tex, 1, GL_RGBA8, image_size.x, image_size.y);
    glBindTextureUnit(1, intermediate_tex);

    GLuint framebuffer = vgl::gl::create_framebuffer();
    glNamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, intermediate_tex, 0);
    vgl::gl::attach_drawbuffers(framebuffer, GL_COLOR_ATTACHMENT0);
    vgl::gl::check_framebuffer(framebuffer);

    auto screen_vao = vgl::gl::create_vertex_array();

    int kernel_size = 0;
    int filter = 0;
    float strength = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (ImGui::Begin("Kernel size")) {
            ImGui::SliderInt("Filter", &filter, 0, 100);
            ImGui::SliderInt("Kernel Size", &kernel_size, 0, 100);
            ImGui::DragFloat("Strength", &strength, 0.01f, 0.0f, 100.0f);
            glProgramUniform1i(first_pass, 0, kernel_size);
            glProgramUniform1i(first_pass, 1, filter);
            glProgramUniform1f(first_pass, 2, strength);
            glProgramUniform1i(second_pass, 0, kernel_size);
            glProgramUniform1i(second_pass, 1, filter);
            glProgramUniform1f(second_pass, 2, strength);
        }
        ImGui::End();
        glUseProgram(first_pass);
        glViewport(0, 0, image_size.x, image_size.y);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glBindVertexArray(screen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(second_pass);
        glViewport(0, 0, w_res.x, w_res.y);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(screen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}
