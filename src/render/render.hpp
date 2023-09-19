#pragma once

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <GLFW/glfw3.h>

namespace render
{
    using shader_stage_type = unsigned long;
    struct Shader final
    {
        std::unordered_map<shader_stage_type, std::string> shader_stage_type_to_path{};
        std::unordered_map<std::string, GLint> uniformsLocations{};
        GLuint program_id{};

        auto compile() -> bool;

        auto apply() -> bool
        {
            if (program_id != 0) {
                glUseProgram(program_id);
                return true;
            } else {
                return false;
            }
        }

        auto setUniform1f(const std::string &uniform_name, float value) -> void;
        auto setUniform1i(const std::string &uniform_name, int value) -> void;
        auto setUniform1ui(const std::string &uniform_name, unsigned int value) -> void;
        auto setUniform1fv(const std::string &uniform_name, GLsizei count, float *value) -> void;
        auto setUniform1iv(const std::string &uniform_name, GLsizei count, int *value) -> void;
        auto setUniform2fv(const std::string &uniform_name, const glm::vec2 &vector) -> void;
        auto setUniform3fv(const std::string &uniform_name, const glm::vec3 &vector) -> void;
        auto setUniform4fv(const std::string &uniform_name, const glm::vec4 &vector) -> void;
        auto setUniformMatrix3fv(const std::string &uniform_name, const glm::mat3 &matrix) -> void;
        auto setUniformMatrix4fv(const std::string &uniform_name, const glm::mat4 &matrix) -> void;

        auto getUniformLocation(const std::string &uniform_name) -> bool;
    };

    namespace window {
        extern GLFWwindow *window;

        static auto swapAndPollInput() -> int
        {
            glfwSwapBuffers(window);
            glfwPollEvents();
            return !glfwWindowShouldClose(window);
        }

        static void glfwErrorCallback(int, const char *message)
        {
            fprintf(stderr, "%s\n", message);
        }
    }

    auto setup_glfw3() -> bool;
} // namespace render
