#include "render/mesh.hpp"
#include "render/render.hpp"

#include <stdio.h>
#include <assert.h>
#include <thread>
#include <format>

int main()
{
    // std::cout << sizeof(glm::dualquat) / sizeof(float) << std::endl;
    auto setup_status = render::setup_glfw3();
    if (!setup_status) {
        return 1;
    }

    assimp_model::Model human_with_skeleton{};

    human_with_skeleton.load_with_config("asset/config.json");

    render::Shader shader {
        {{GL_VERTEX_SHADER, "asset/shaders/Basic.vert"}, {GL_FRAGMENT_SHADER, "asset/shaders/Basic.frag"},}
    };
    // "#define MAX_bone_id_and_weight_LEN " + std::format("{:d}\n", human_with_skeleton.uniform_mesh.bone_id_and_weight.size()
    assert(shader.compile() == true);
    assert(shader.apply() == true);
    shader.setUniform1i("bone_id_and_weight", 0);
    shader.setUniform1i("bone_bind_pose", 1);

    shader.setUniform1i("bone_current_pose", 2 + human_with_skeleton.play_anim_track);
    // shader.compile();

    auto world_matrix      = glm::mat4(1.0f);
    
    auto projection_matrix = glm::perspectiveFov(glm::radians(60.0f), float(1024), float(768), 0.1f, 10.0f);

    auto time{0.0f};
    auto last_clock = clock();
    auto frame_id{0};

    auto weight_left_frame{1.0f};
    auto weight_right_frame{0.0f};

    

    auto display = [&]()
    {
        auto view_matrix  = glm::lookAt(render::window::cam_position, render::window::cam_look_at, render::window::cam_up);
        auto world_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(-0.0f), glm::vec3(1, 0, 0));
        world_matrix = glm::rotate(world_matrix, glm::radians(0.0f), glm::vec3(0, 0, 1));
        world_matrix = glm::scale(world_matrix, glm::vec3(human_with_skeleton.scale));
        world_matrix = glm::translate(world_matrix, glm::vec3(0, -40, -10));

        assert(shader.apply() == true);

        time += float(clock() - last_clock) / float(CLOCKS_PER_SEC);
        last_clock = clock();

        auto& track = human_with_skeleton.tracks[human_with_skeleton.play_anim_track];

        auto frame_time{1.0f / human_with_skeleton.speed / track.frame_per_second};

        weight_right_frame = time / frame_time;
        weight_left_frame = 1.0f - weight_right_frame;

        // shader.setUniform1f("time", time);
        shader.setUniformMatrix4fv("world", world_matrix);
        // shader.setUniformMatrix3fv("normalMatrix", glm::inverse(glm::transpose(glm::mat3(world_matrix))));
        shader.setUniformMatrix4fv("viewProj", projection_matrix * view_matrix);
        shader.setUniform3fv("cam_pos", render::window::cam_position);
        shader.setUniform1i("frame_id", frame_id);
        shader.setUniform1f("left_weight", weight_left_frame);
        shader.setUniform1f("right_weight", weight_right_frame);
    
        shader.apply();
        human_with_skeleton.draw();

        if (time >= frame_time) {
            frame_id++;
            time = 0.0f;
            last_clock = clock();
        }
        
        if (frame_id > track.duration - 1) {
            frame_id = 0;
        }
    };

    auto run = [&]()
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        do
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            ImGui::Begin("Demo window");
            ImGui::Button("Hello!");
            ImGui::End();
            ImGui::Render();
            // int display_w, display_h;
            // assert(render::window::window != nullptr);
            glfwGetFramebufferSize(render::window::window, &render::window::SCR_WIDTH, &render::window::SCR_HEIGHT);
            projection_matrix = glm::perspectiveFov(glm::radians(60.0f), float(render::window::SCR_WIDTH), float(render::window::SCR_HEIGHT), 0.1f, 10.0f);
            glViewport(0, 0, render::window::SCR_WIDTH, render::window::SCR_HEIGHT);
            display();

            

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        } while (render::window::swapAndPollInput());
    };

    std::cout << "start loop\n";

    // Event loop
    run();

    // Cleanup
    glfwTerminate();
    return 0;
}
