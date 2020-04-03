#include "vgl/core/window.hpp"
#include "vgl/core/gui.hpp"
#include "vgl/file/file.hpp"
#include "vgl/gpu_api/gl/shader.hpp"
#include "vgl/gpu_api/gl/buffer.hpp"
#include "vgl/gpu_api/gl/vao.hpp"
#include "vgl/gpu_api/gl/texture.hpp"
#include "vgl/file/image_file.hpp"
#include "vgl/gpu_api/gl/framebuffer.hpp"
#include "vgl/gpu_api/gl/debug.hpp"
#include "vgl/rendering/scene.hpp"
#include "vgl/rendering/camera.hpp"
#include "vgl/rendering/light.hpp"
#include <glsp/preprocess.hpp>
#include <chrono>
#include <random>

// enable optimus!
using DWORD = uint32_t;

extern "C" {
_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}


float gen_random_float(const float lower, const float upper) {
    std::random_device rd;
    std::mt19937 eng(rd());
    const std::uniform_real_distribution<float> uni(lower, upper);
    return uni(eng);
}

Eigen::Vector4f gen_random_vec4(const float lower = 0.0f, const float upper = 1.0f, const float w = 1.0f) {
    return Eigen::Vector4f(gen_random_float(lower, upper), gen_random_float(lower, upper),
                          gen_random_float(lower, upper), w);
}

std::pair<Eigen::Vector3f, Eigen::Vector3f> plane_vecs(Eigen::Vector3f const& n) {
    Eigen::Vector3f w(0, 0, 1);
    Eigen::Vector3f n_abs = n.cwiseAbs();
    if (n_abs.x() < std::min(n_abs.y(), n_abs.z())) {
        w = Eigen::Vector3f(1, 0, 0);
    }
    else if (n_abs.y() < std::min(n_abs.x(), n_abs.z())) {
        w = Eigen::Vector3f(0, 1, 0);
    }
    Eigen::Vector3f u = w.cross(n).normalized();
    Eigen::Vector3f v = n.cross(u).normalized();
    return {u, v};
}

struct G_buffer {
    vgl::gl::glframebuffer fbo;
    vgl::gl::gltexture color;
    vgl::gl::gltexture specular;
    vgl::gl::gltexture pos;
    vgl::gl::gltexture normal;
    vgl::gl::gltexture depth;
    void init(Eigen::Vector2i const& size) {
        fbo = vgl::gl::create_framebuffer();
        color = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(color, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(color, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(color, 1, GL_RGBA32F, size.x(), size.y());
        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, color, 0);
        specular = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(specular, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(specular, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(specular, 1, GL_RGBA32F, size.x(), size.y());
        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT1, specular, 0);
        normal = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(normal, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(normal, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(normal, 1, GL_RGB32F, size.x(), size.y());
        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT2, normal, 0);
        pos = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(pos, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(pos, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(pos, 1, GL_RGB32F, size.x(), size.y());
        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT3, pos, 0);
        depth = vgl::gl::create_texture(GL_TEXTURE_2D);
        glTextureParameteri(depth, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(depth, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(depth, 1, GL_DEPTH24_STENCIL8, size.x(), size.y());
        glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, depth, 0);
        vgl::gl::attach_draw_buffers(
            fbo, {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3});
        vgl::gl::check_framebuffer(fbo);
    }
};

int main() {
    Eigen::Vector2i w_res(1600, 900);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    vgl::Window window(w_res.x(), w_res.y(), "Hello");
    window.enable_gl();
    glfwSwapInterval(0);
    glViewport(0, 0, w_res.x(), w_res.y());
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(vgl::gl::debug_callback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_MULTISAMPLE);
    vgl::ui::Gui gui(window);
    {
        auto& style = ImGui::GetStyle();
        style.WindowRounding = 0;
        style.ScrollbarRounding = 0.0f;
        style.ScrollbarSize = 20.0f;
        style.DisplaySafeAreaPadding = ImVec2(0.0f, 0.0f);
        style.DisplayWindowPadding = ImVec2(0.0f, 0.0f);
        style.ChildBorderSize = 1.0f;
        ImGui::StyleColorsVS();
        glClearColor(0.43f, 0.43f, 0.43f, 1.0f);
    }

    vgl::Fly_through_controller cam_controller(
        [&window](Eigen::Vector3d& dir) {
            if (!ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive()) {
                dir = Eigen::Vector3d(
                    static_cast<double>(window.key[GLFW_KEY_D]) - static_cast<double>(window.key[GLFW_KEY_A]),
                    static_cast<double>(window.key[GLFW_KEY_E]) - static_cast<double>(window.key[GLFW_KEY_Q]),
                    static_cast<double>(window.key[GLFW_KEY_S]) - static_cast<double>(window.key[GLFW_KEY_W]));
                if (dir.squaredNorm() != 0.0f) {
                    if (window.key[GLFW_KEY_SPACE]) {
                        dir *= 10.0;
                    }
                    return true;
                }
            }
            return false;
        },
        [&window](Eigen::Vector2d& angles) {
            if (!ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive()) {
                if (window.mouse[GLFW_MOUSE_BUTTON_LEFT]) {
                    angles = math::to_radians(Eigen::Vector2d(window.cursor_delta.y(), window.cursor_delta.x()));
                    return true;
                }
            }
            return false;
        });
    vgl::GLCamera cam;
    cam_controller.set_rotate_speed(3.0);
    cam.projection = vgl::perspective_projection<double>(16.0 / 9.0, 90.0, 0.001, 100.0);

    auto cam_ssbo = vgl::gl::create_buffer(cam.get_cam_data(), GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cam_ssbo);

    vgl::gl::glprogram phong, depth_prepass, lights_debug, aabb_debug, screen, raytracing;

    auto reload_shaders = [&]() {
        auto raytracing_vs_future = vgl::file::load_string_file_async(vgl::file::shaders_path / "shading/raytracing.vert");
        auto raytracing_fs_future = vgl::file::load_string_file_async(vgl::file::shaders_path / "shading/raytracing.frag");
        auto depth_prepass_vs_future = vgl::file::load_string_file_async(vgl::file::shaders_path / "shading/depth_prepass.vert");
        auto depth_prepass_fs_future = vgl::file::load_string_file_async(vgl::file::shaders_path / "shading/depth_prepass.frag");
        auto lights_debug_vs_ftr = vgl::file::load_string_file_async(vgl::file::shaders_path / "debug/lights.vert");
        auto lights_debug_fs_ftr = vgl::file::load_string_file_async(vgl::file::shaders_path / "debug/lights.frag");
        auto screen_vs_ftr = vgl::file::load_string_file_async(vgl::file::shaders_path / "util/screen.vert");
        auto screen_fs_ftr = vgl::file::load_string_file_async(vgl::file::shaders_path / "util/screen.frag");
        auto aabb_debug_vs_source_ftr = vgl::file::load_string_file_async(vgl::file::shaders_path / "debug/bb.vert");
        auto aabb_debug_gs_source_ftr = vgl::file::load_string_file_async(vgl::file::shaders_path / "debug/bb.geom");
        auto aabb_debug_fs_source_ftr = vgl::file::load_string_file_async(vgl::file::shaders_path / "debug/red.frag");

        auto phong_vs_source = glsp::preprocess_file(vgl::file::shaders_path / "shading/shading.vert").contents;
        auto phong_fs_source = glsp::preprocess_file(vgl::file::shaders_path / "shading/phong.frag").contents;
        auto phong_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, phong_vs_source);
        auto phong_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, phong_fs_source);
        phong = vgl::gl::create_program({ phong_vs, phong_fs });

        auto raytracing_vs_source = glsp::preprocess_source(raytracing_vs_future.get(), "raytracing.vert",
            { (vgl::file::shaders_path / "shading").string() }).contents;
        auto raytracing_fs_source = glsp::preprocess_source(raytracing_fs_future.get(), "raytracing.frag",
            { (vgl::file::shaders_path / "shading").string() }).contents;
        auto raytracing_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, raytracing_vs_source);
        auto raytracing_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, raytracing_fs_source);
        raytracing = vgl::gl::create_program({ raytracing_vs, raytracing_fs });

        auto screen_vs_source = glsp::preprocess_source(screen_vs_ftr.get(), "screen.vert",
            { (vgl::file::shaders_path / "util").string() }).contents;
        auto screen_fs_source = glsp::preprocess_source(screen_fs_ftr.get(), "screen.frag",
            { (vgl::file::shaders_path / "util").string() }).contents;
        auto screen_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, screen_vs_source);
        auto screen_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, screen_fs_source);
        screen = vgl::gl::create_program({ screen_vs, screen_fs });
        vgl::gl::delete_shader(screen_vs);
        vgl::gl::delete_shader(screen_fs);

        auto depth_prepass_vs_source = glsp::preprocess_source(depth_prepass_vs_future.get(), "depth_prepass.vert",
            { (vgl::file::shaders_path / "shading").string() }).contents;
        auto depth_prepass_fs_source = glsp::preprocess_source(depth_prepass_fs_future.get(), "depth_prepass.frag",
            { (vgl::file::shaders_path / "shading").string() }).contents;
        auto depth_prepass_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, depth_prepass_vs_source);
        auto depth_prepass_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, depth_prepass_fs_source);
        depth_prepass = vgl::gl::create_program({ depth_prepass_vs, depth_prepass_fs });
        vgl::gl::delete_shader(depth_prepass_vs);
        vgl::gl::delete_shader(depth_prepass_fs);

        auto lights_vs_source = glsp::preprocess_source(lights_debug_vs_ftr.get(), "lights.vert",
            { (vgl::file::shaders_path / "debug").string() }).contents;
        auto lights_fs_source = glsp::preprocess_source(lights_debug_fs_ftr.get(), "lights.frag",
            { (vgl::file::shaders_path / "debug").string() }).contents;
        auto lights_debug_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, lights_vs_source);
        auto lights_debug_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, lights_fs_source);
        lights_debug = vgl::gl::create_program({ lights_debug_vs, lights_debug_fs });
        vgl::gl::delete_shader(lights_debug_vs);
        vgl::gl::delete_shader(lights_debug_fs);

        auto aabb_debug_vs_source = glsp::preprocess_source(aabb_debug_vs_source_ftr.get(), "bb.vert",
            { (vgl::file::shaders_path / "debug").string() }).contents;
        auto aabb_debug_gs_source = glsp::preprocess_source(aabb_debug_gs_source_ftr.get(), "bb.geom",
            { (vgl::file::shaders_path / "debug").string() }).contents;
        auto aabb_debug_fs_source = glsp::preprocess_source(aabb_debug_fs_source_ftr.get(), "red.frag",
            { (vgl::file::shaders_path / "debug").string() }).contents;
        auto aabb_debug_vs = vgl::gl::create_shader(GL_VERTEX_SHADER, aabb_debug_vs_source);
        auto aabb_debug_gs = vgl::gl::create_shader(GL_GEOMETRY_SHADER, aabb_debug_gs_source);
        auto aabb_debug_fs = vgl::gl::create_shader(GL_FRAGMENT_SHADER, aabb_debug_fs_source);

        aabb_debug = vgl::gl::create_program({ aabb_debug_vs, aabb_debug_gs, aabb_debug_fs });
        vgl::gl::delete_shader(aabb_debug_vs);
        vgl::gl::delete_shader(aabb_debug_gs);
        vgl::gl::delete_shader(aabb_debug_fs);

    };
    reload_shaders();

    G_buffer g_buffer_one{};


    Eigen::Vector2i fb_res;
    glfwGetFramebufferSize(window.get(), &fb_res.x(), &fb_res.y());

    g_buffer_one.init(fb_res);

    window.cbs.window_size["resize"] = [&](GLFWwindow*, int x, int y) {
        glViewport(0, 0, x, y);
        w_res.x() = x;
        w_res.y() = y;
    };
    window.cbs.framebuffer_size["resize"] = [&](GLFWwindow*, int x, int y) {
        fb_res.x() = x;
        fb_res.y() = y;
        g_buffer_one.init(fb_res);
    };

    auto debug_vao = vgl::gl::create_vertex_array();

    vgl::Scene scene{};
    bool mesh_loaded = false;

    std::vector<vgl::Light> lights{vgl::Light{Eigen::Vector4f(0, 0, 0, 1.0), Eigen::Vector4f::Ones(),
                                              Eigen::Vector4f(0.0, -1.0, 0.0, 0.0), vgl::Attenuation{},
                                              math::pi<float> / 4.0f, 1, 1, 0}};

    vgl::gl::glbuffer model_vbo = 0;
    vgl::gl::glbuffer indices_buffer = 0;
    vgl::gl::glbuffer draw_indirect_buffer = 0;
    vgl::gl::glvertexarray model_vao = 0;
    vgl::gl::glbuffer material_buffer = 0;
    vgl::gl::glbuffer mat_info_buffer = 0;
    vgl::gl::glbuffer tex_ref_buffer = 0;
    vgl::gl::glbuffer bounds_buffer = 0;
    vgl::gl::glbuffer triangle_buffer = 0;
    vgl::gl::glbuffer vertices_buffer = 0;

    std::vector<vgl::gl::gltexture> textures;

    auto lights_ssbo = vgl::gl::create_buffer(lights, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lights_ssbo);

    auto screen_vao = vgl::gl::create_vertex_array();

    using high_res_clock = std::chrono::high_resolution_clock;
    auto current = high_res_clock::now();
    auto previous = high_res_clock::now();
    float dt = 0.0f;
    float accumulator = 0.0f;
    int frames = 0;
    float ms_p_f = 0.0f;
    while (!window.should_close()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        dt = std::chrono::duration<float>(current - previous).count();
        accumulator += dt;
        if (accumulator > 1.0f) {
            ms_p_f = accumulator / static_cast<float>(frames);
            frames = 0;
            accumulator = 0.0;
        }
        frames++;
        previous = current;
        current = high_res_clock::now();
        gui.start_frame();
        bool lights_added_or_removed = false;
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::MenuItem("Load Mesh")) {
                auto file = vgl::file::open_file_dialog(vgl::file::resources_path / "models");
                if (file) {
                    textures.clear();
                    glFinish();
                    scene = vgl::load_scene(file.value());
                    textures.resize(scene.textures.size());
                    auto tex_data = vgl::file::load_images(scene.textures, true);
                    std::vector<GLuint64> tex_handles(scene.textures.size());
                    for (auto i = 0; i < scene.textures.size(); ++i) {
                        textures.at(i) = vgl::gl::create_texture(GL_TEXTURE_2D);
                        auto tex_format = vgl::gl::derive_internal_format(tex_data.at(i));
                        auto desc = vgl::img::get_image_desc(tex_data.at(i));
                        glTextureParameteri(textures.at(i), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTextureParameteri(textures.at(i), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTextureParameteri(textures.at(i), GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
                        glTextureParameteri(textures.at(i), GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
                        glTextureStorage2D(textures.at(i), 1, tex_format, desc.width, desc.height);
                        vgl::gl::set_texture_data_2d(textures.at(i), tex_data.at(i));
                        tex_handles.at(i) = vgl::gl::get_texture_handle(textures.at(i));
                    }
                    mesh_loaded = true;
                    indices_buffer = vgl::gl::create_buffer(scene.indices);
                    /*struct Vertex {
                        glm::vec4 pos;
                        glm::vec4 normal;
                        glm::vec2 uv;
                        glm::vec2 pad;
                    };
                    std::vector<Vertex> vertices(scene.vertices.size());
                    for (int i = 0; i < vertices.size(); ++i) {
                        vertices.at(i).pos = glm::vec4(scene.vertices.at(i).pos, 1.0f);
                        vertices.at(i).normal = glm::vec4(scene.vertices.at(i).normal, 0.0f);
                        vertices.at(i).uv = glm::vec2(scene.vertices.at(i).uv);
                    }*/
                    //vertices_buffer = vgl::gl::create_buffer(vertices);
                    model_vbo = vgl::gl::create_buffer(scene.vertices);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, model_vbo);
                    struct Triangle {
                        Eigen::Vector3ui idx;
                        int obj_id = 0;
                    };
                    std::vector<Triangle> triangles(scene.indices.size() / 3);
                    for (int i = 0; i < triangles.size(); i++) {
                        triangles.at(i).idx = { scene.indices.at(i * 3),scene.indices.at(i * 3 + 1), scene.indices.at(i * 3 + 2) };
                    }
                    int obj_id = 0;
                    for (auto& ob : scene.draw_cmds) {
                        for (int i = ob.first_index / 3; i < (ob.first_index + ob.count) / 3; ++i) {
                            triangles.at(i).idx.array() += ob.base_vertex;
                            triangles.at(i).obj_id = obj_id;
                        }
                        obj_id++;
                    }
                    triangle_buffer = vgl::gl::create_buffer(triangles);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, triangle_buffer);
                    draw_indirect_buffer = vgl::gl::create_buffer(scene.draw_cmds);
                    material_buffer = vgl::gl::create_buffer(scene.materials);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, material_buffer);
                    mat_info_buffer = vgl::gl::create_buffer(scene.objects);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mat_info_buffer);
                    tex_ref_buffer = vgl::gl::create_buffer(tex_handles);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, tex_ref_buffer);
                    bounds_buffer = vgl::gl::create_buffer(scene.object_bounds);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bounds_buffer);
                    model_vao = vgl::gl::create_vertex_array();
                    glVertexArrayVertexBuffer(model_vao, 0, model_vbo, 0,
                        static_cast<int>(vgl::sizeof_value_type(scene.vertices)));
                    glVertexArrayElementBuffer(model_vao, indices_buffer);
                    glEnableVertexArrayAttrib(model_vao, 0);
                    glEnableVertexArrayAttrib(model_vao, 1);
                    glEnableVertexArrayAttrib(model_vao, 2);
                    glVertexArrayAttribFormat(model_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(vgl::Vertex, pos));
                    glVertexArrayAttribFormat(model_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(vgl::Vertex, normal));
                    glVertexArrayAttribFormat(model_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(vgl::Vertex, uv));
                    glVertexArrayAttribBinding(model_vao, 0, 0);
                    glVertexArrayAttribBinding(model_vao, 1, 0);
                    glVertexArrayAttribBinding(model_vao, 2, 0);
                }
            }
            if (ImGui::MenuItem("Reload Shaders")) {
                reload_shaders();
            }
        }
        ImGui::EndMainMenuBar();
        if (ImGui::Begin("Settings")) {
            if (ImGui::CollapsingHeader("Light settings")) {
                if (ImGui::Button("Add light")) {
                    lights.push_back(vgl::Light{});
                    lights_added_or_removed = true;
                }
                int i = 0;
                for (auto light_it = lights.begin(); light_it != lights.end();) {
                    if (ImGui::CollapsingHeader(("Light " + std::to_string(i)).c_str())) {
                        ImGui::Combo(("Type##light" + std::to_string(i)).c_str(), &light_it->type,
                                     "Ambient\0Point\0Directional\0Spotlight\0\0");
                        ImGui::Text("Type: %d", light_it->type);
                        ImGui::ColorEdit3(("Color##light" + std::to_string(i)).c_str(), light_it->color.data(),
                                          ImGuiColorEditFlags_Float);
                        ImGui::DragFloat(("Brightness##light" + std::to_string(i)).c_str(), &light_it->color.w(), 0.1f,
                                         0.0f, 100.0f);
                        ImGui::DragFloat3(("Attenuation##light" + std::to_string(i)).c_str(),
                                          reinterpret_cast<float*>(&light_it->attenuation), 0.0001f, 0.0f, 100.0f,
                                          "%.5f");
                        if (light_it->type == 1 || light_it->type == 3) {
                            ImGui::DragFloat3(("Position##light" + std::to_string(i)).c_str(), light_it->pos.data(),
                                              0.01f);
                        }
                        if (light_it->type == 2 || light_it->type == 3) {
                            ImGui::DragFloat3(("Direction##light" + std::to_string(i)).c_str(), light_it->dir.data(),
                                              0.01f, -1.0f, 1.0f);
                        }
                        if (light_it->type == 3) {
                            ImGui::DragFloat(("Cutoff##light" + std::to_string(i)).c_str(), &light_it->outer_cutoff,
                                             0.01f, 0.0f, math::pi<float>);
                        }
                        if (ImGui::Button(("Delete##light" + std::to_string(i)).c_str())) {
                            light_it = lights.erase(light_it);
                            lights_added_or_removed = true;
                        }
                    }
                    if (lights.end() == light_it) {
                        break;
                    }
                    ++light_it;
                    ++i;
                }
            }
        }
        ImGui::End();

        if (lights_added_or_removed) {
            lights_ssbo = vgl::gl::create_buffer(lights, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lights_ssbo);
            glFinish();
        }
        //if (mesh_loaded) {
            cam_controller.update_camera(cam, dt);
            const auto lights_ptr = glMapNamedBufferRange(lights_ssbo, 0, sizeof(vgl::Light) * lights.size(),
                GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
            std::memcpy(lights_ptr, lights.data(), sizeof(vgl::Light) * lights.size());
            glUnmapNamedBuffer(lights_ssbo);
            if (!ImGui::IsAnyItemHovered() && !ImGui::IsAnyWindowHovered() && !ImGui::IsAnyItemFocused()
                && !ImGui::IsAnyItemActive()) {
                if (window.key[GLFW_KEY_P]) {
                    cam.reset();
                }
                if (window.key[GLFW_KEY_R]) {
                    reload_shaders();
                }
                const auto buffer_ptr = glMapNamedBufferRange(cam_ssbo, 0, sizeof(vgl::Cam_data),
                    GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
                std::memcpy(buffer_ptr, &cam.get_cam_data(), sizeof(vgl::Cam_data));
                glUnmapNamedBuffer(cam_ssbo);
            }
            //glBindFramebuffer(GL_FRAMEBUFFER, g_buffer_one.fbo);
            //glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            //glUseProgram(depth_prepass);
            //glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                //static_cast<unsigned int>(scene.objects.size()), 0);
            //glDepthFunc(GL_EQUAL);
            
            /*else {
                glBindVertexArray(model_vao);
                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_indirect_buffer);
                glUseProgram(phong);
                glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                    static_cast<unsigned int>(scene.objects.size()), 0);
            }*/
            //glDepthFunc(GL_LESS);
            //glDisable(GL_DEPTH_TEST);
            //glBindFramebuffer(GL_FRAMEBUFFER, 0);
            //glBindVertexArray(screen_vao);
            //glProgramUniformHandleui64ARB(screen, glGetUniformLocation(screen, "tex"), vgl::gl::get_texture_handle(g_buffer_one.color));
            //glUseProgram(screen);
            //glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 1, 0);
            //glEnable(GL_DEPTH_TEST);
            /*glBindVertexArray(debug_vao);
            glUseProgram(aabb_debug);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawArraysInstancedBaseInstance(GL_LINES, 0, 2, static_cast<int>(scene.object_bounds.size()), 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);*/
            if (window.key[GLFW_KEY_X]) {
                glBindVertexArray(screen_vao);
                glUseProgram(raytracing);
                glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, 1, 0);
            }
            else {
                glBindVertexArray(model_vao);
                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_indirect_buffer);
                glUseProgram(phong);
                glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                    static_cast<unsigned int>(scene.objects.size()), 0);
            }
            glDepthFunc(GL_GREATER);
            glBindVertexArray(screen_vao);
            glUseProgram(lights_debug);
            glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, lights.size(), 0);
            glDepthFunc(GL_LESS);
        //}
        gui.render();
        window.swap_buffers();
        if (!ImGui::IsAnyItemHovered() && !ImGui::IsAnyWindowHovered() && !ImGui::IsAnyItemFocused()
            && !ImGui::IsAnyItemActive()) {
            if (window.mouse[GLFW_MOUSE_BUTTON_LEFT]) {
                glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else {
                glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
        window.poll_events();
    }
}
