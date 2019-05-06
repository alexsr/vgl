#pragma once

#include "glad/glad.h"
#include "vgl/file/file.hpp"
#include <string>


namespace vgl
{
    namespace gl
    {
        struct Specialization_constant {
            GLuint index{};
            GLuint value{};
        };
        GLuint create_shader_spirv(GLenum type, const std::filesystem::path& binary_path) {
            auto binary = vgl::load_binary_file(binary_path);
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

        GLuint create_shader_spirv(GLenum type, const std::vector<std::byte>& binary) {
            GLuint shader = glCreateShader(type);
            glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, binary.data(), static_cast<int>(binary.size()));
            return shader;
        }

        void specialize_shader(GLuint shader, const std::vector<Specialization_constant>& constants = {}) {
            const auto c_size = static_cast<unsigned int>(constants.size());
            std::vector<GLuint> c_indices(constants.size());
            std::vector<GLuint> c_values(constants.size());
            for (unsigned int i = 0; i < c_size; i++) {
                c_indices.at(i) = constants.at(i).index;
                c_values.at(i) = constants.at(i).value;
            }
            glSpecializeShader(shader, "main", c_size, c_indices.data(), c_values.data());
            GLint compile_status = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
            if (compile_status == GL_FALSE) {
                GLint log_size = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);
                std::string error_log(static_cast<unsigned long>(log_size), ' ');
                glGetShaderInfoLog(shader, log_size, &log_size, error_log.data());
                glDeleteShader(shader);
                throw std::runtime_error{
                    "Error specializing shader: " + std::to_string(shader) + "\n"
                    + "Error log: \n"
                    + error_log
                };
            }
        }

        void specialize_shaders(std::initializer_list<GLuint> shader, const std::vector<Specialization_constant>& constants = {}) {
            for (const auto& s : shader) {
                specialize_shader(s, constants);
            }
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

        void delete_shaders(std::initializer_list<GLuint> shaders) {
            for (auto s : shaders) {
                if (glIsShader(s)) {
                    glDeleteShader(s);
                }
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
