#pragma once

#include "glad/glad.h"
#include "vgl/file/file.hpp"
#include <string>


namespace vgl
{
    namespace gl
    {
        GLuint create_shader_spirv(GLenum type, const std::filesystem::path& binary_path) {
            auto binary = load_binary_file(binary_path);
            GLuint shader = glCreateShader(type);
            glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, binary.data(), static_cast<int>(binary.size()));
            glSpecializeShader(shader, "main", 0, nullptr, nullptr);
            GLint compile_status = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
            if (compile_status == GL_FALSE) {
                GLint log_size = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);
                std::string error_log(static_cast<unsigned long>(log_size), ' ');
                glGetShaderInfoLog(shader, log_size, &log_size, error_log.data());
                glDeleteShader(shader);
                throw std::runtime_error{
                    "Error specializing shader: " + binary_path.string() + "\n"
                    + "Error log: \n"
                    + error_log
                };
            }
            return shader;
        }

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

        GLuint create_shader(const GLenum type, const std::string& source) {
            const GLuint shader = glCreateShader(type);
            const GLchar* source_ptr = source.data();
            auto size = static_cast<GLint>(source.size());
            glShaderSource(shader, 1, &source_ptr, &size);
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

        void delete_shader(GLuint shader) {
            if (glIsShader(shader)) {
                glDeleteShader(shader);
            }
        }

        void delete_shaders(std::initializer_list<GLuint> shaders) {
            for (auto s : shaders) {
                delete_shader(s);
            }
        }

        void delete_program(GLuint program) {
            if (glIsProgram(program)) {
                glDeleteProgram(program);
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

        GLuint create_program(std::initializer_list<GLuint> shaders) {
            const GLuint program = glCreateProgram();
            attach_shaders(program, shaders);
            link_program(program);
            detach_shaders(program, shaders);
            return program;
        }
    }
}
