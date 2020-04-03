#pragma once

#include "handle.hpp"
#include <string>
#include <filesystem>
#include <glsp/glsp.hpp>
#include "vgl/math/eigen_utils.hpp"

namespace vgl::gl {
inline void compile_shader(GLuint const shader) noexcept(false) {
    glCompileShader(shader);
    GLint compile_status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status == GL_FALSE) {
        GLint log_size = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);
        std::string error_log;
        error_log.resize(static_cast<unsigned long>(log_size));
        glGetShaderInfoLog(shader, log_size, &log_size, error_log.data());
        glDeleteShader(shader);
        throw std::runtime_error{"Error while compiling shader from string.\nError log: \n" + error_log};
    }
}

inline glshader create_shader(GLenum const type, std::string const& source) {
    GLuint const shader = glCreateShader(type);
    GLchar const* source_ptr = source.data();
    auto const size = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &source_ptr, &size);
    compile_shader(shader);
    return shader;
}

inline glshader create_shader(GLenum const type, char const* source, size_t const size) {
    GLuint const shader = glCreateShader(type);
    auto const size_i = static_cast<GLint>(size);
    glShaderSource(shader, 1, &source, &size_i);
    compile_shader(shader);
    return shader;
}

inline glshader create_shader_from_file(GLenum const type, std::filesystem::path const& file_path) {
    auto source = glsp::preprocess_file(file_path).contents;
    return create_shader(type, source);
}

inline void attach_shaders(GLuint const program, std::initializer_list<GLuint> const& shaders) {
    for (auto const& s : shaders) {
        glAttachShader(program, s);
    }
}

inline void detach_shaders(GLuint const program, std::initializer_list<GLuint> const& shaders) {
    for (auto const& s : shaders) {
        glDetachShader(program, s);
    }
}

inline void link_program(GLuint const program) {
    glLinkProgram(program);
    GLint link_status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string error_log;
        error_log.resize(static_cast<unsigned long>(length));
        glGetProgramInfoLog(program, length, &length, &error_log[0]);
        glDeleteProgram(program);
        throw std::runtime_error{"Error while linking shader program " + std::to_string(program) + "\n" +
                                 "Error log: \n" + error_log};
    }
}

inline glprogram create_program(std::initializer_list<GLuint> const& shaders) {
    GLuint const program = glCreateProgram();
    attach_shaders(program, shaders);
    link_program(program);
    detach_shaders(program, shaders);
    return program;
}

template <typename T>
void update_uniform(GLuint const program, GLint const location, T const& value) {
    std::cout << "Uniform type " << typeid(T).name() << "does not exist.\n";
}

template <>
inline void update_uniform(GLuint const program, GLint const location, GLuint64 const& value) {
    glProgramUniformHandleui64ARB(program, location, value);
}

template <>
inline void update_uniform(GLuint const program, GLint const location, std::vector<GLuint64> const& values) {
    glProgramUniformHandleui64vARB(program, location, static_cast<GLsizei>(values.size()), values.data());
}

template <>
inline void update_uniform(GLuint const program, GLint location, bool const& value) {
    glProgramUniform1i(program, location, value);
}

template <>
inline void update_uniform(GLuint const program, GLint const location, int const& value) {
    glProgramUniform1i(program, location, value);
}

template <>
inline void update_uniform(GLuint const program, GLint const location, Eigen::Vector2i const& value) {
    glProgramUniform2i(program, location, value.x(), value.y());
}

template <>
inline void update_uniform(GLuint const program, GLint const location, Eigen::Vector3i const& value) {
    glProgramUniform3i(program, location, value.x(), value.y(), value.z());
}

template <>
inline void update_uniform(GLuint const program, GLint const location, Eigen::Vector4i const& value) {
    glProgramUniform4i(program, location, value.x(), value.y(), value.z(), value.w());
}

template <>
inline void update_uniform(GLuint const program, GLint const location, std::uint32_t const& value) {
    glProgramUniform1ui(program, location, value);
}

template <>
inline void update_uniform(GLuint const program, GLint const location, Eigen::Vector2ui const& value) {
    glProgramUniform2ui(program, location, value.x(), value.y());
}

template <>
inline void update_uniform(GLuint const program, GLint const location, Eigen::Vector3ui const& value) {
    glProgramUniform3ui(program, location, value.x(), value.y(), value.z());
}

template <>
inline void update_uniform(GLuint const program, GLint const location, Eigen::Vector4ui const& value) {
    glProgramUniform4ui(program, location, value.x(), value.y(), value.z(), value.w());
}

template <>
inline void update_uniform(GLuint const program, GLint const location, float const& value) {
    glProgramUniform1f(program, location, value);
}

template <>
inline void update_uniform(GLuint const program, GLint const location, Eigen::Vector2f const& value) {
    glProgramUniform2f(program, location, value.x(), value.y());
}

template <>
inline void update_uniform(GLuint const program, GLint const location, Eigen::Vector3f const& value) {
    glProgramUniform3f(program, location, value.x(), value.y(), value.z());
}

template <>
inline void update_uniform(GLuint const program, GLint const location, Eigen::Vector4f const& value) {
    glProgramUniform4f(program, location, value.x(), value.y(), value.z(), value.w());
}

template <>
inline void update_uniform(GLuint const program, GLint const location, Eigen::Matrix2f const& value) {
    glProgramUniformMatrix2fv(program, location, 1, GL_FALSE, value.data());
}

template <>
inline void update_uniform(GLuint const program, GLint const location, Eigen::Matrix3f const& value) {
    glProgramUniformMatrix3fv(program, location, 1, GL_FALSE, value.data());
}

template <>
inline void update_uniform(GLuint const program, GLint const location, Eigen::Matrix4f const& value) {
    glProgramUniformMatrix4fv(program, location, 1, GL_FALSE, value.data());
}
} // namespace vgl::gl
