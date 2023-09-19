#include "render.hpp"
#include "render/cmake-source-dir.hpp"
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <assert.h>

namespace render
{
    auto Shader::compile() -> bool
    {
        auto load_file = [](std::string filename) -> std::string
        {
            if (filename.empty())
                return "";
            std::string filetext;
            std::string line;
            std::ifstream inFile(ROOT_DIR + filename);

            if (!inFile)
            {
                inFile.close();
                return "";
            }
            else
            {
                std::stringstream shader_stream{};
                shader_stream << inFile.rdbuf();
                filetext = shader_stream.str();
                // while (getline(inFile, line))
                //     filetext.append(line + "\n");
                inFile.close();
                return filetext;
            }
        };

        program_id = glCreateProgram();
        for (auto &t2p : shader_stage_type_to_path)
        {
            auto shader_stage_type = t2p.first;
            auto &shader_path = t2p.second;
            auto shader_obj = glCreateShader(shader_stage_type);
            auto shader_code = load_file(shader_path);
            // assert(shader_code.size() > 0);
            const char* shader_code_cc = shader_code.c_str();
            glShaderSource(shader_obj, 1, &shader_code_cc, nullptr);
            glCompileShader(shader_obj);

            auto result{GL_TRUE};
            glGetShaderiv(shader_obj, GL_COMPILE_STATUS, &result);

            if (result == GL_FALSE)
            {
                // std::cout << "compile error " + shader_path << std::endl;
                char infoLog[512];
                glGetShaderInfoLog(shader_obj, 512, nullptr, infoLog);
                std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
                return false;
            }

            glAttachShader(program_id, shader_obj);
            glDeleteShader(shader_obj);
        }

        glLinkProgram(program_id);

        auto status{GL_TRUE};
        glGetProgramiv(program_id, GL_LINK_STATUS, &status);

        if (status == GL_FALSE)
        {
            // std::cout << "shader link error" << std::endl;
            char infoLog[512];
            glGetProgramInfoLog(program_id, 512, nullptr, infoLog);
            std::cout << "ERROR::SHADER::LINK_FAILED\n" << infoLog << std::endl;
            return false;
        }

        return true;
    }

    auto Shader::setUniform1f(const std::string &uniform_name, float value) -> void
    {
        if (uniformsLocations.count(uniform_name))
        {
            glProgramUniform1f(program_id, uniformsLocations[uniform_name], value);
        }
        else if (getUniformLocation(uniform_name))
        {
            glProgramUniform1f(program_id, uniformsLocations[uniform_name], value);
        }
    }

    auto Shader::setUniform1i(const std::string &uniform_name, int value) -> void
    {
        if (uniformsLocations.count(uniform_name))
        {
            glProgramUniform1i(program_id, uniformsLocations[uniform_name], value);
        }
        else if (getUniformLocation(uniform_name))
        {
            glProgramUniform1i(program_id, uniformsLocations[uniform_name], value);
        }
    }

    auto Shader::setUniform1ui(const std::string &uniform_name, unsigned int value) -> void
    {
        if (uniformsLocations.count(uniform_name))
        {
            glProgramUniform1ui(program_id, uniformsLocations.at(uniform_name), value);
        }
        else if (getUniformLocation(uniform_name))
        {
            glProgramUniform1ui(program_id, uniformsLocations[uniform_name], value);
        }
    }

    auto Shader::setUniform1fv(const std::string &uniform_name, GLsizei count, float *value) -> void
    {
        if (uniformsLocations.count(uniform_name))
        {
            glProgramUniform1fv(program_id, uniformsLocations[uniform_name], count, value);
        }
        else
        {
            if (getUniformLocation(uniform_name))
            {
                glProgramUniform1fv(program_id, uniformsLocations[uniform_name], count, value);
            }
        }
    }

    auto Shader::setUniform1iv(const std::string &uniform_name, GLsizei count, int *value) -> void
    {
        if (uniformsLocations.count(uniform_name))
        {
            glProgramUniform1iv(program_id, uniformsLocations[uniform_name], count, value);
        }
        else
        {
            if (getUniformLocation(uniform_name))
            {
                glProgramUniform1iv(program_id, uniformsLocations[uniform_name], count, value);
            }
        }
    }

    auto Shader::setUniform2fv(const std::string &uniform_name, const glm::vec2 &vector) -> void
    {
        if (uniformsLocations.count(uniform_name))
        {
            glProgramUniform2fv(program_id, uniformsLocations[uniform_name], 1, glm::value_ptr(vector));
        }
        else if (getUniformLocation(uniform_name))
        {
            glProgramUniform2fv(program_id, uniformsLocations[uniform_name], 1, glm::value_ptr(vector));
        }
    }

    auto Shader::setUniform3fv(const std::string &uniform_name, const glm::vec3 &vector) -> void
    {
        if (uniformsLocations.count(uniform_name))
        {
            glProgramUniform3fv(program_id, uniformsLocations[uniform_name], 1, glm::value_ptr(vector));
        }
        else if (getUniformLocation(uniform_name))
        {
            glProgramUniform3fv(program_id, uniformsLocations[uniform_name], 1, glm::value_ptr(vector));
        }
    }

    auto Shader::setUniform4fv(const std::string &uniform_name, const glm::vec4 &vector) -> void
    {
        if (uniformsLocations.count(uniform_name))
        {
            glProgramUniform4fv(program_id, uniformsLocations[uniform_name], 1, glm::value_ptr(vector));
        }
        else if (getUniformLocation(uniform_name))
        {
            glProgramUniform4fv(program_id, uniformsLocations[uniform_name], 1, glm::value_ptr(vector));
        }
    }

    auto Shader::setUniformMatrix3fv(const std::string &uniform_name, const glm::mat3 &matrix) -> void
    {
        if (uniformsLocations.count(uniform_name))
        {
            glProgramUniformMatrix3fv(program_id, uniformsLocations[uniform_name], 1, GL_FALSE, glm::value_ptr(matrix));
        }
        else if (getUniformLocation(uniform_name))
        {
            glProgramUniformMatrix3fv(program_id, uniformsLocations[uniform_name], 1, GL_FALSE, glm::value_ptr(matrix));
        }
    }

    auto Shader::setUniformMatrix4fv(const std::string &uniform_name, const glm::mat4 &matrix) -> void
    {
        if (uniformsLocations.count(uniform_name))
        {
            glProgramUniformMatrix4fv(program_id, uniformsLocations[uniform_name], 1, GL_FALSE, glm::value_ptr(matrix));
        }
        else if (getUniformLocation(uniform_name))
        {
            glProgramUniformMatrix4fv(program_id, uniformsLocations[uniform_name], 1, GL_FALSE, glm::value_ptr(matrix));
        }
    }
    auto Shader::getUniformLocation(const std::string &uniform_name) -> bool
    {
        GLint uniform_location = glGetUniformLocation(program_id, uniform_name.c_str());

        if (uniform_location != -1)
        {
            uniformsLocations[uniform_name] = uniform_location;
            return true;
        }
        else
        {
            fprintf(stderr, "Error! Can't find uniform %s\n", uniform_name.c_str());
            return false;
        }
    }

    GLFWwindow* window::window{nullptr};

    auto setup_glfw3() -> bool
    {
        glfwInit();
        glfwSetErrorCallback(window::glfwErrorCallback);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, GLFW_LOSE_CONTEXT_ON_RESET);
        window::window = glfwCreateWindow(1024, 768, "GL test app", NULL, NULL);
        if (window::window != nullptr) {
            glfwMakeContextCurrent(window::window);
            return true;
        } else {
            return false;
        }
    }
} // namespace render
