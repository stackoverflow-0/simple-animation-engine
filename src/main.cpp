#include "render/mesh.hpp"
#include "render/render.hpp"

#include <stdio.h>
#include <assert.h>

int main()
{
    auto setup_status = render::setup_glfw3();
    if (!setup_status) {
        return 1;
    }

    if (glewInit() != GLEW_OK)
        throw std::runtime_error("glewInit failed");

    assimp_model::Model human_with_skeleton{};

    human_with_skeleton.load_model("asset/models/human-with-anim.fbx");
    

    render::Shader shader{
        {{GL_VERTEX_SHADER, "asset/shaders/Basic.vert"}, {GL_FRAGMENT_SHADER, "asset/shaders/Basic.frag"},}
    };

    assert(shader.compile() == true);
    // shader.compile();

    auto cam_position = glm::vec3(0.0f, 1.0f, 1.2f);
    auto cam_look_at  = glm::vec3(0.0f, 0.5f, 0.0f);
    auto cam_up       = glm::vec3(0.0f, 1.0f, 0.0f);

    auto world_matrix      = glm::mat4(1.0f);
    auto view_matrix       = glm::lookAt(cam_position, cam_look_at, cam_up);
    auto projection_matrix = glm::perspectiveFov(glm::radians(60.0f), float(1024), float(768), 0.1f, 10.0f);
    
    auto time{0.0f};
    auto display = [&]()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        auto world_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(-1, 0, 0));
        // world_matrix = glm::rotate(world_matrix, time * glm::radians(-90.0f), glm::vec3(0, 0, 1));
        world_matrix = glm::scale(world_matrix, glm::vec3{0.005f});
        assert(shader.apply() == true);
        time += 0.01f;
        shader.setUniformMatrix4fv("world", world_matrix);
        shader.setUniformMatrix3fv("normalMatrix", glm::inverse(glm::transpose(glm::mat3(world_matrix))));
        shader.setUniformMatrix4fv("viewProj", projection_matrix * view_matrix);
        shader.setUniform3fv("cam_pos", cam_position);
        shader.apply();

        human_with_skeleton.draw();
    };

    auto run = [&]()
    {
        glClearColor(0.6f, 0, 0.3f, 0);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        do
        {
            int display_w, display_h;
            // assert(render::window::window != nullptr);
            glfwGetFramebufferSize(render::window::window, &display_w, &display_h);
            projection_matrix = glm::perspectiveFov(glm::radians(60.0f), float(display_w), float(display_h), 0.1f, 10.0f);
            glViewport(0, 0, display_w, display_h);
            display();
        } while (render::window::swapAndPollInput());
    };

    // Event loop
    run();
    // Cleanup
    glfwTerminate();
    return 0;
}