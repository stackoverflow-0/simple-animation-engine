#include "render.hpp"
#include "render/cmake-source-dir.hpp"


// #include <imgui/imgui_impl_opengl3_loader.h>

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <assert.h>

namespace render
{
    auto Shader::compile(const std::string& marco) -> bool
    {
        auto load_file = [&](std::string filename) -> std::string
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
                return filetext.insert(14, marco);
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

    auto Shader::setUniform1b(const std::string &uniform_name, bool value) -> void
    {
        if (uniformsLocations.count(uniform_name))
        {
            glProgramUniform1i(program_id, uniformsLocations[uniform_name], value ? 1 : 0);
        }
        else if (getUniformLocation(uniform_name))
        {
            glProgramUniform1i(program_id, uniformsLocations[uniform_name], value ? 1 : 0);
        }
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

    // auto Shader::createUniformBuffer(const std::string &uniform_name, const std::vector<glm::vec2>& buffer) -> GLuint
    // {
    //     auto bone_buffer_id = glGetUniformBlockIndex(program_id, "uniform_name");
    //     if (bone_buffer_id != -1) {
    //         GLint blockSize{};
    //         glGetActiveUniformBlockiv(program_id, bone_buffer_id, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
    //         GLuint ubo{};
    //         glGenBuffers(1, &ubo);
    //         // glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    //         // glBufferData(GL_UNIFORM_BUFFER, blockSize, buffer.data(), GL_DYNAMIC_DRAW);
    //         // glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
    //         return ubo;
    //     }
    // }

    // auto Shader::setUniformBuffer(const std::string &uniform_name, const std::vector<glm::vec2>& buffer, GLuint ubo) -> void
    // {
    //     glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    //     glBufferSubData(GL_UNIFORM_BUFFER, 0, buffer.size() * sizeof(glm::vec2), buffer.data());
    //     glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
    // }

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

namespace window{ 
    GLFWwindow* window{nullptr};

    glm::vec3 cam_position = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 cam_look_at  = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cam_up       = glm::vec3(0.0f, 1.0f, 0.0f);

    int SCR_WIDTH = 1024;
    int SCR_HEIGHT = 768;

    double xpos_old{};
    double ypos_old{};

    float yaw{};
    float pitch{};

    void mouse_callback(GLFWwindow *window, double xpos, double ypos)
    {
        if(glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS) {
            xpos_old = xpos;
            ypos_old = ypos;
            return;
        }

        yaw   += (( xpos - xpos_old) / float(SCR_WIDTH)) * 3.14;
        pitch += ((-ypos + ypos_old) / float(SCR_HEIGHT)) * 3.14;

        xpos_old = xpos;
        ypos_old = ypos;

        glm::vec3 direction;
        direction.x = sin(yaw) * cos(pitch);
        direction.y = sin(pitch);
        direction.z = -cos(yaw) * cos(pitch);
        cam_position = -5.0f * glm::normalize(direction);

        auto _cam_position = -5.0f * glm::normalize(glm::vec3{sin(yaw) * cos(pitch - 0.1f), sin(pitch - 0.1f), -cos(yaw) * cos(pitch - 0.1f)});

        cam_up = glm::normalize(_cam_position - cam_position);
        // glfwSetCursorPos(window, SCR_WIDTH / 2, SCR_HEIGHT / 2);
        return;
    }}

    auto setup_glfw3() -> bool
    {
        glfwInit();
        glfwSetErrorCallback(window::glfwErrorCallback);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, GLFW_LOSE_CONTEXT_ON_RESET);
        window::window = glfwCreateWindow(1024, 768, "Skeleton Animation App", NULL, NULL);
        if (window::window != nullptr) {
            
            glfwSetCursorPosCallback(window::window, window::mouse_callback);
            glfwMakeContextCurrent(window::window);

            if (glewInit() != GLEW_OK)
                throw std::runtime_error("glewInit failed");
            // imgl3wInit();

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            // Setup Platform/Renderer bindings
            // assert(ImGui_ImplGlfw_InitForOpenGL(render::window::window, true));
            if(ImGui_ImplGlfw_InitForOpenGL(render::window::window, true) != true) {
                std::cout << "init glfw imgui failed\n";
                // return false;
            }
            // assert(ImGui_ImplOpenGL3_Init("#version 430"));

            if(ImGui_ImplOpenGL3_Init("#version 430") != true) {
                // fprintf(stderr, "Failed to initialize OpenGL loader!\n");
                std::cout << "init opengl3 imgui failed\n";
                // return false;
            }
            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            // return false;

            // IMGUI_CHECKVERSION();
            // ImGui::CreateContext();
            // ImGuiIO& io = ImGui::GetIO(); (void)io;
            // ImGui::StyleColorsDark();
            // ImGui_ImplGlfw_InitForOpenGL(window::window, true);
            // ImGui_ImplOpenGL3_Init("#version 150");

            // while (!glfwWindowShouldClose(window::window))
            // {
            //     glClear(GL_COLOR_BUFFER_BIT);

            //     ImGui_ImplOpenGL3_NewFrame();
            //     ImGui_ImplGlfw_NewFrame();
            //     ImGui::NewFrame();

            //     ImGui::ShowDemoWindow();

            //     ImGui::Render();
            //     ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            //     glfwSwapBuffers(window::window);
            //     glfwPollEvents();
            // }

            // ImGui_ImplOpenGL3_Shutdown();
            // ImGui_ImplGlfw_Shutdown();
            // ImGui::DestroyContext();

            // glfwDestroyWindow(window::window);
            // glfwTerminate();
            return true;
        } else {
            return false;
        }

        
        
    }
} // namespace render
