#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "stb/stb_image.h"
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"
#include "glm/glm.hpp"
#include "vgl/file/file.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include <execution>
#include "glm/gtc/matrix_transform.hpp"
#include <fstream>
#include <array>
#include <iostream>

struct Context {
	GLint major_version{};
	GLint minor_version{};
	std::string vendor;
	std::string renderer;
	std::string extended_glsl_version;
	std::string glsl_version;

	GLint binary_format_count;
	std::vector<GLint> binary_formats;

	//    // Buffer information

	//    GLint max_ubo_bindings;
	//    GLint max_ssbo_bindings;

	GLint max_framebuffer_width{};
	GLint max_framebuffer_height{};
	GLint max_framebuffer_layers{};
	GLint max_framebuffer_samples{};
	GLint max_renderbuffer_size{};
	GLint max_color_attachments{};

	GLint max_texture_size{};
	GLint max_array_texture_layers{};
	GLint max_cube_map_texture_size{};
	GLint max_rectangle_texture_size{};
	GLint max_texture_buffer_size{};
	GLint max_3d_texture_size{};
	GLint max_texture_lod_bias{};
	GLint max_color_texture_samples{};
	GLint max_depth_texture_samples{};

	std::array<GLint, 3> max_workgroup_count{};
	std::array<GLint, 3> max_workgroup_size{};
	GLint max_workgroup_invocations{};
	GLint max_compute_shared_memory_size{};

	void retrieve_info() {
	glGetIntegerv(GL_MAJOR_VERSION, &major_version);
	glGetIntegerv(GL_MINOR_VERSION, &minor_version);
	vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	extended_glsl_version = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	glsl_version = std::to_string(major_version * 100 + minor_version * 10);

	glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &binary_format_count);
	binary_formats.resize(binary_format_count);
	for (int i = 0; i < binary_format_count; i++) {
		glGetIntegeri_v(GL_SHADER_BINARY_FORMATS, i, &binary_formats.at(i));
	}

	// framebuffer queries
	glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &max_framebuffer_width);
	glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &max_framebuffer_height);
	glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, &max_framebuffer_layers);
	glGetIntegerv(GL_MAX_FRAMEBUFFER_SAMPLES, &max_framebuffer_samples);
	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer_size);
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_array_texture_layers);
	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &max_cube_map_texture_size);
	glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE, &max_rectangle_texture_size);
	glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &max_texture_buffer_size);
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_3d_texture_size);
	glGetIntegerv(GL_MAX_TEXTURE_LOD_BIAS, &max_texture_lod_bias);
	glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &max_color_texture_samples);
	glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &max_depth_texture_samples);

	// compute shader queries
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &max_workgroup_count.at(0));
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &max_workgroup_count.at(1));
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &max_workgroup_count.at(2));
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_workgroup_size.at(0));
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &max_workgroup_size.at(1));
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &max_workgroup_size.at(2));
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_workgroup_invocations);
	glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &max_compute_shared_memory_size);
}
	void print_info() {
		std::cout << "OpenGL Version: " << major_version << "." << minor_version;
		std::cout << "Vendor: " << vendor;
		std::cout << "Renderer: " << renderer;
		std::cout << "GLSL Version: " << extended_glsl_version;
	}
};


struct Mesh {
	std::vector<glm::vec4> vertices;
	std::vector<glm::vec4> normals;
	std::vector<glm::vec2> uvs;
};

glm::vec4 to_glm_vec4(const aiVector3D& v, float w = 1.0f) {
	return glm::vec4(v.x, v.y, v.z, w);
}

std::vector<std::byte> load_file_binary(const std::filesystem::path& file_path) {
	std::ifstream f(file_path, std::ios::binary | std::ios::in);
	const auto file_size = std::filesystem::file_size(file_path);
	std::vector<std::byte> buffer(file_size);
	f.read(reinterpret_cast<char*>(buffer.data()), file_size);
	return buffer;
}

Mesh load_mesh(const std::filesystem::path& file_path) {
	Assimp::Importer importer;
	auto scene = importer.ReadFile(file_path.string(), 0);

	auto mesh = scene->mMeshes[0];
	Mesh res{};
	res.vertices.resize(mesh->mNumVertices);
	res.normals.resize(mesh->mNumVertices);
	res.uvs.resize(mesh->mNumVertices);
	std::transform(std::execution::par, mesh->mVertices, mesh->mVertices + mesh->mNumVertices,
		std::begin(res.vertices), [](const aiVector3D& v) {
			return to_glm_vec4(v, 1.0f);
	});
	if (mesh->HasNormals()) {
		std::transform(std::execution::par, mesh->mNormals, mesh->mNormals + mesh->mNumVertices,
			std::begin(res.normals), [](const aiVector3D& v) {
				return to_glm_vec4(v, 0.0f);
			});
	}
	if (mesh->HasTextureCoords(0)) {
		std::transform(std::execution::par, mesh->mTextureCoords[0], mesh->mTextureCoords[0] + mesh->mNumVertices,
			std::begin(res.uvs), [](const aiVector3D& v) {
				return glm::vec2(v.x, v.y);
			});
	}
	return res;
}

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

	Context ctx;
	ctx.retrieve_info();
	ctx.print_info();
	glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	struct Cam_mats {
		glm::mat4 model{};
		glm::mat4 view{};
		glm::mat4 proj{};
	} cam;

	cam.model = glm::mat4(1.0f);
	cam.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	cam.proj = glm::perspective(glm::radians(60.0f), 1.6f, 0.001f, 100.0f);

	GLuint cam_ssbo = 0;
	glCreateBuffers(1, &cam_ssbo);
	glNamedBufferStorage(cam_ssbo, sizeof(cam), &cam, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cam_ssbo);

	auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	auto vert_source = load_file_binary(vgl::shaders_path / "minimal/minimal.vert");
	glShaderBinary(1, &vertex_shader, GL_SHADER_BINARY_FORMAT_SPIR_V, vert_source.data(), static_cast<int>(vert_source.size()));
	glSpecializeShader(vertex_shader, "main", 0, nullptr, nullptr);
	GLint status = 0;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
	auto frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	auto frag_source = load_file_binary(vgl::shaders_path / "minimal/minimal.frag");
	glShaderBinary(1, &frag_shader, GL_SHADER_BINARY_FORMAT_SPIR_V, frag_source.data(), frag_source.size());
	glSpecializeShader(frag_shader, "main", 0, nullptr, nullptr);
	glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &status);

	const GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);
	GLint is_linked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &is_linked);
	if (is_linked == GL_FALSE) {
		GLint length = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		std::string info_log(length, ' ');
		glGetProgramInfoLog(program, length, &length, info_log.data());
		std::cout << info_log;
		glDeleteProgram(program);
	}

	glDetachShader(program, vertex_shader);
	glDetachShader(program, frag_shader);
	glDeleteShader(vertex_shader);
	glDeleteShader(frag_shader);

	Mesh mesh {};
	bool mesh_loaded = false;

    while (!glfwWindowShouldClose(window) && !mesh_loaded) {
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
		if (ImGui::Begin("Import Mesh")) {
			if (ImGui::Button("Load Mesh")) {
				auto file = vgl::open_file_dialog(vgl::resources_path);
				if (file) {
					mesh = load_mesh(file.value());
					mesh_loaded = true;
				}
			}
		}
		ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

	GLuint model_vbo = 0;
	glCreateBuffers(1, &model_vbo);
	glNamedBufferStorage(model_vbo, mesh.vertices.size() * sizeof(decltype(mesh.vertices)::value_type), mesh.vertices.data(), 0);

	GLuint model_vao = 0;
	glCreateVertexArrays(1, &model_vao);
	glEnableVertexArrayAttrib(model_vao, 0);
	glVertexArrayVertexBuffer(model_vao, 0, model_vbo, 0, sizeof(decltype(mesh.vertices)::value_type));
	glVertexArrayAttribFormat(model_vao, 0, 4, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(model_vao, 0, 0);
	glUseProgram(program);

	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(model_vao);
		glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(mesh.vertices.size()));
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

    glfwTerminate();
}
