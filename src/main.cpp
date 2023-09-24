#include "render/mesh.hpp"
#include "render/render.hpp"

#include <stdio.h>
#include <assert.h>
#include <thread>
#include <format>

int main()
{
    std::cout << sizeof(glm::dualquat) / sizeof(float) << std::endl;
    auto setup_status = render::setup_glfw3();
    if (!setup_status) {
        return 1;
    }

    if (glewInit() != GLEW_OK)
        throw std::runtime_error("glewInit failed");

    assimp_model::Model human_with_skeleton{};

    human_with_skeleton.load_model("asset/models/human-with-anim-good.fbx");

    // "asset/models/human-with-anim.fbx"
    // auto human_with_skeleton_load_model = [&]() -> void {
    //     human_with_skeleton.load_model("asset/models/human-with-anim.fbx");
    // };

    // auto human_with_skeleton_load_model_thd = std::thread(human_with_skeleton_load_model);

    render::Shader shader {
        {{GL_VERTEX_SHADER, "asset/shaders/Basic.vert"}, {GL_FRAGMENT_SHADER, "asset/shaders/Basic.frag"},}
    };
    // "#define MAX_bone_id_and_weight_LEN " + std::format("{:d}\n", human_with_skeleton.uniform_mesh.bone_id_and_weight.size()
    assert(shader.compile() == true);
    assert(shader.apply() == true);
    shader.setUniform1i("bone_id_and_weight", 0);
    shader.setUniform1i("bone_bind_pose", 1);
    shader.setUniform1i("bone_current_pose", 4);
    // shader.compile();

    auto world_matrix      = glm::mat4(1.0f);
    
    auto projection_matrix = glm::perspectiveFov(glm::radians(60.0f), float(1024), float(768), 0.1f, 10.0f);

    auto time{0.0f};
    auto frame_id{0};
    auto display = [&]()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        auto view_matrix  = glm::lookAt(render::window::cam_position, render::window::cam_look_at, render::window::cam_up);
        auto world_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(-0.0f), glm::vec3(1, 0, 0));
        world_matrix = glm::rotate(world_matrix, glm::radians(0.0f), glm::vec3(0, 0, 1));
        world_matrix = glm::scale(world_matrix, glm::vec3{0.01f});
        world_matrix = glm::translate(world_matrix, glm::vec3(0, 0, -10));

        assert(shader.apply() == true);

        time += 0.01f;
        if (time > 0.05f) {
            frame_id++;
            time = 0.0f;
        }
        
        if (frame_id >= 56) {
            frame_id = 0;
        }
        
        // shader.setUniform1f("time", time);
        shader.setUniformMatrix4fv("world", world_matrix);
        // shader.setUniformMatrix3fv("normalMatrix", glm::inverse(glm::transpose(glm::mat3(world_matrix))));
        shader.setUniformMatrix4fv("viewProj", projection_matrix * view_matrix);
        shader.setUniform3fv("cam_pos", render::window::cam_position);
        shader.setUniform1i("frame_id", frame_id);
        shader.apply();
        human_with_skeleton.draw();
    };

    auto run = [&]()
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0);
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

    std::cout << "start loop\n";

    // Event loop
    run();

    // Cleanup
    glfwTerminate();
    return 0;
}
