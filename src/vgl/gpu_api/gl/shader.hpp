#pragma once

#include "vgl/file/file.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "handle.hpp"
#include <string>

namespace vgl::gl {
    void compile_shader(const GLuint shader) {
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
            throw std::runtime_error{
                "Error while compiling shader from string.\nError log: \n"
                + error_log
            };
        }
    }

    glshader create_shader(const GLenum type, const std::string& source) {
        const GLuint shader = glCreateShader(type);
        const GLchar* source_ptr = source.data();
        auto size = static_cast<GLint>(source.size());
        glShaderSource(shader, 1, &source_ptr, &size);
        compile_shader(shader);
        return shader;
    }

    glshader create_shader(const GLenum type, const char* source, size_t size) {
        const GLuint shader = glCreateShader(type);
        auto size_i = static_cast<GLint>(size);
        glShaderSource(shader, 1, &source, &size_i);
        compile_shader(shader);
        return shader;
    }

    void attach_shaders(GLuint program, std::initializer_list<GLuint> shaders) {
        for (auto s : shaders) {
            glAttachShader(program, s);
        }
    }

    void detach_shaders(GLuint program, std::initializer_list<GLuint> shaders) {
        for (auto s : shaders) {
            glDetachShader(program, s);
        }
    }

    void link_program(GLuint program) {
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
            throw std::runtime_error{
                "Error while linking shader program " + std::to_string(program) + "\n"
                + "Error log: \n"
                + error_log
            };
        }
    }

    glprogram create_program(std::initializer_list<GLuint> shaders) {
        const GLuint program = glCreateProgram();
        attach_shaders(program, shaders);
        link_program(program);
        detach_shaders(program, shaders);
        return program;
    }

    template <typename T>
    void update_uniform(const GLuint program, GLint location, const T& value) {
        log_error() << "Uniform type " << typeid(T).name() << "does not exist.\n";
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const GLuint64& value) {
        glProgramUniformHandleui64ARB(program, location, value);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const std::vector<GLuint64>& values) {
        glProgramUniformHandleui64vARB(program, location, static_cast<GLsizei>(values.size()), values.data());
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const bool& value) {
        glProgramUniform1i(program, location, value);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const int& value) {
        glProgramUniform1i(program, location, value);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const glm::ivec2& value) {
        glProgramUniform2i(program, location, value.x, value.y);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const glm::ivec3& value) {
        glProgramUniform3i(program, location, value.x, value.y, value.z);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const glm::ivec4& value) {
        glProgramUniform4i(program, location, value.x, value.y, value.z, value.w);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const glm::uint& value) {
        glProgramUniform1ui(program, location, value);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const glm::uvec2& value) {
        glProgramUniform2ui(program, location, value.x, value.y);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const glm::uvec3& value) {
        glProgramUniform3ui(program, location, value.x, value.y, value.z);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const glm::uvec4& value) {
        glProgramUniform4ui(program, location, value.x, value.y, value.z, value.w);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const float& value) {
        glProgramUniform1f(program, location, value);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const glm::vec2& value) {
        glProgramUniform2f(program, location, value.x, value.y);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const glm::vec3& value) {
        glProgramUniform3f(program, location, value.x, value.y, value.z);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const glm::vec4& value) {
        glProgramUniform4f(program, location, value.x, value.y, value.z, value.w);
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const glm::mat2& value) {
        glProgramUniformMatrix2fv(program, location, 1, GL_FALSE, value_ptr(value));
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const glm::mat3& value) {
        glProgramUniformMatrix3fv(program, location, 1, GL_FALSE, value_ptr(value));
    }

    template <>
    inline void update_uniform(const GLuint program, GLint location, const glm::mat4& value) {
        glProgramUniformMatrix4fv(program, location, 1, GL_FALSE, value_ptr(value));
    }
}
