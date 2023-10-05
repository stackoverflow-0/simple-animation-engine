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
    shader.compile();
    shader.apply();
    shader.setUniform1i("bone_id_and_weight", 0);
    shader.setUniform1i("bone_bind_pose", 1);

    render::Shader gizmo_shader {
        {{GL_VERTEX_SHADER, "asset/shaders/Gizmo.vert"}, {GL_FRAGMENT_SHADER, "asset/shaders/Gizmo.frag"},}
    };
    gizmo_shader.compile();
    gizmo_shader.apply();
    // gizmo_shader.setUniform1i("bone_bind_pose", 1);

    assimp_model::Model gizmo_model{};

    gizmo_model.load_with_config("asset/gizmo_config.json");

    
    // shader.compile();

    auto world_matrix      = glm::mat4(1.0f);
    
    auto projection_matrix = glm::perspectiveFov(glm::radians(60.0f), float(1024), float(768), 0.1f, 10.0f);

    auto time{0.0f};
    auto last_clock = clock();
    auto frame_id{0};

    auto weight_left_frame{1.0f};
    auto weight_right_frame{0.0f};

    auto speed{1.0f};

    auto show_bone_gizmo{false};

    auto bone_gizmo_color = glm::vec4(0.0f, 1.0f, 0.7f, 1.0f);

    auto display = [&]()
    {
        auto view_matrix  = glm::lookAt(render::window::cam_position, render::window::cam_look_at, render::window::cam_up);
        auto world_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(-0.0f), glm::vec3(1, 0, 0));
        world_matrix = glm::rotate(world_matrix, glm::radians(0.0f), glm::vec3(0, 0, 1));
        world_matrix = glm::translate(world_matrix, glm::vec3(0, -0.4, -0.1));
        world_matrix = glm::scale(world_matrix, glm::vec3(human_with_skeleton.scale));

        auto gizmo_world_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.4, -0.1));
        gizmo_world_matrix = glm::scale(gizmo_world_matrix, glm::vec3(human_with_skeleton.scale));

        shader.apply();
        shader.setUniformMatrix4fv("world", world_matrix);
        shader.setUniformMatrix4fv("viewProj", projection_matrix * view_matrix);
        shader.setUniform3fv("cam_pos", render::window::cam_position);

        gizmo_shader.apply();
        gizmo_shader.setUniformMatrix4fv("world", gizmo_world_matrix);
        gizmo_shader.setUniformMatrix4fv("viewProj", projection_matrix * view_matrix);
        // gizmo_shader.setUniform3fv("cam_pos", render::window::cam_position);

        if (! human_with_skeleton.import_animation) {
            shader.apply();
            human_with_skeleton.draw();
            return;
        }

        if (human_with_skeleton.speed > 0.0f) {
            time += float(clock() - last_clock) / float(CLOCKS_PER_SEC);
            speed = human_with_skeleton.speed;
        }
        last_clock = clock();

        auto& track = human_with_skeleton.tracks[human_with_skeleton.play_anim_track];

        auto frame_time{1.0f / speed / track.frame_per_second};

        weight_right_frame = time / frame_time;
        weight_left_frame = 1.0f - weight_right_frame;

        shader.apply();
        shader.setUniform1b("import_animation", human_with_skeleton.import_animation);
        shader.setUniform1i("frame_id", frame_id);
        shader.setUniform1f("left_weight", weight_left_frame);
        shader.setUniform1f("right_weight", weight_right_frame);
        shader.setUniform1i("bone_current_pose", 2 + human_with_skeleton.play_anim_track);
        shader.setUniform1i("show_bone_weight_id", human_with_skeleton.show_bone_weight_id);
        
        gizmo_shader.apply();
        gizmo_shader.setUniform1i("frame_id", frame_id);
        gizmo_shader.setUniform1f("left_weight", weight_left_frame);
        gizmo_shader.setUniform1f("right_weight", weight_right_frame);
        gizmo_shader.setUniform1i("bone_current_pose", 2 + human_with_skeleton.play_anim_track);
        gizmo_shader.setUniform4fv("gizmo_color", bone_gizmo_color);
        gizmo_shader.setUniform1f("gizmo_scale", gizmo_model.scale);

        if (frame_id > track.duration - 1) {
            frame_id = 0;
        }
    
        shader.apply();
        human_with_skeleton.draw();
        if (show_bone_gizmo) {
            glDisable(GL_DEPTH_TEST);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
            gizmo_shader.apply();
            for (auto bone_gz_id = 0; bone_gz_id < human_with_skeleton.bones.size(); bone_gz_id++) {
                gizmo_shader.setUniform1i("bone_id", bone_gz_id);
                gizmo_model.draw();
            }
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
        }

        if (time >= frame_time) {
            frame_id++;
            time = 0.0f;
            last_clock = clock();
        }
    };

    auto run = [&]()
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        auto human_with_skeleton_config_scale = human_with_skeleton.scale;
        auto gizmo_model_config_scale = gizmo_model.scale;
        
        do
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                auto anim_tool_active{true};
                ImGui::Begin("Animation Tools", &anim_tool_active, ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar);
                if (human_with_skeleton.import_animation) {
                    ImGui::SliderFloat("speed", &human_with_skeleton.speed, 0.0f, 2.0f);
                    ImGui::SliderFloat("scale", &human_with_skeleton.scale, 0.0f, human_with_skeleton_config_scale * 2.0f);
                    ImGui::Text("%d - %s", human_with_skeleton.play_anim_track, human_with_skeleton.tracks[human_with_skeleton.play_anim_track].track_name.c_str());
                    ImGui::SliderInt("track", &human_with_skeleton.play_anim_track, 0, human_with_skeleton.tracks.size() - 1);
                    if (human_with_skeleton.show_bone_weight_id >= 0)
                        ImGui::Text("%d - %s", human_with_skeleton.show_bone_weight_id, human_with_skeleton.bones[human_with_skeleton.show_bone_weight_id].name.c_str());
                    else
                        ImGui::Text("Disable bone weight visualize");
                    ImGui::SliderInt("bone", &human_with_skeleton.show_bone_weight_id, -1, human_with_skeleton.bones.size() - 1);
                    ImGui::Checkbox("show bone gizmo", &show_bone_gizmo);
                    
                    if (show_bone_gizmo) {
                        ImGui::SliderFloat("gizmo size", &gizmo_model.scale, 0.0f, 5.0f);
                        ImGui::ColorPicker4("gizmo color", reinterpret_cast<float*>(&bone_gizmo_color));
                    }
                    // ImGui::SliderFloat("gizmo scale", &gizmo_model.scale, 0.0f, gizmo_model_config_scale * 2.0f);
                }
                ImGui::End();
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwGetFramebufferSize(render::window::window, &render::window::SCR_WIDTH, &render::window::SCR_HEIGHT);
            projection_matrix = glm::perspectiveFov(glm::radians(60.0f), float(render::window::SCR_WIDTH), float(render::window::SCR_HEIGHT), 0.1f, 10.0f);
            glViewport(0, 0, render::window::SCR_WIDTH, render::window::SCR_HEIGHT);
 
            display();
            
        } while (render::window::swapAndPollInput());
    };

    std::cout << "start loop\n";

    // Event loop
    run();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
