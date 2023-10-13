#include "render/mesh.hpp"
#include "render/render.hpp"

#include <stdio.h>
#include <assert.h>
#include <thread>
#include <format>

#include <chrono>

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
    auto last_clock = std::chrono::high_resolution_clock().now();
    auto frame_ids = std::vector<int>{};
    frame_ids.resize(human_with_skeleton.tracks.size());

    auto blend_weights = std::vector<float>{1.0f, 0.0f, 0.0f};
    auto track_tex_id = std::vector<int>{};
    for (auto id = 0; id < human_with_skeleton.tracks.size(); id++) {
        track_tex_id.emplace_back(id + 2);
    }

    auto weight_left_frame{1.0f};
    auto weight_right_frame{0.0f};

    auto speed{1.0f};

    auto show_bone_gizmo{false};
    auto show_blend_space{false};

    auto bone_gizmo_color = glm::vec4(0.0f, 1.0f, 0.7f, 1.0f);

    auto delta_frame_time{0.0f};

    auto fps{0};
    auto final_fps{0};

    auto fps_sec{0.0f};

    auto display = [&]()
    {
        auto view_matrix  = glm::lookAt(render::window::cam_position, render::window::cam_look_at, render::window::cam_up);
        auto world_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(-0.0f), glm::vec3(1, 0, 0));
        world_matrix = glm::rotate(world_matrix, glm::radians(0.0f), glm::vec3(0, 0, 1));
        world_matrix = glm::translate(world_matrix, glm::vec3(0, -0.4, -0.1));
        world_matrix = glm::scale(world_matrix, glm::vec3(human_with_skeleton.scale));

        // auto gizmo_world_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.4, -0.1));
        // gizmo_world_matrix = glm::scale(gizmo_world_matrix, glm::vec3(human_with_skeleton.scale));

        shader.apply();
        shader.setUniformMatrix4fv("world", world_matrix);
        shader.setUniformMatrix4fv("viewProj", projection_matrix * view_matrix);
        shader.setUniform3fv("cam_pos", render::window::cam_position);

        gizmo_shader.apply();
        gizmo_shader.setUniformMatrix4fv("world", world_matrix);
        gizmo_shader.setUniformMatrix4fv("viewProj", projection_matrix * view_matrix);
        auto current_clock = std::chrono::high_resolution_clock().now();
        // std::time(& current_clock);
        delta_frame_time = double(std::chrono::duration_cast<std::chrono::nanoseconds>(current_clock - last_clock).count()) * 1e-9;

        last_clock = current_clock;

        fps++;
        fps_sec += delta_frame_time;
        if (fps_sec >= 1.0f) {
            fps_sec = 0.0f;
            final_fps = fps;
            fps = 0;
        }
        if (! human_with_skeleton.import_animation) {

            shader.apply();
            human_with_skeleton.draw();
            return;
        }

        if (human_with_skeleton.speed > 0.0f) {
            // time += delta_frame_time;
            speed = human_with_skeleton.speed;
        }

        auto& track = human_with_skeleton.tracks[human_with_skeleton.play_anim_track];

        auto frame_time{1.0f / speed / track.frame_per_second};

        for (auto id = 0; id < frame_ids.size(); id++) {
            auto& frame_id = frame_ids[id];
            if (frame_id >= human_with_skeleton.tracks[id].duration) {
                frame_id = 0;
                weight_right_frame = 0.0f;
            }
        }

        if (human_with_skeleton.speed > 0.0f) {
            weight_right_frame += delta_frame_time / frame_time;
            weight_left_frame = 1.0f - weight_right_frame;
        }

        if (!show_blend_space) {
            for (auto i = 0; i < blend_weights.size(); i++) {
                if (i == human_with_skeleton.play_anim_track) {
                    blend_weights[i] = 1.0f;
                } else {
                    blend_weights[i] = 0.0f;
                }
            }
        }

        shader.apply();
        shader.setUniform1b("import_animation", human_with_skeleton.import_animation);
        shader.setUniform1iv("frame_ids", 3, frame_ids.data());
        shader.setUniform1f("left_weight", weight_left_frame);
        shader.setUniform1f("right_weight", weight_right_frame);

        shader.setUniform1iv("bone_current_poses", human_with_skeleton.tracks.size(), track_tex_id.data());
        shader.setUniform1i("blend_anim_num", 3);
        shader.setUniform1fv("blend_weights", 3, blend_weights.data());
        // shader.setUniform1i("bone_current_pose", 2 + human_with_skeleton.play_anim_track);
        shader.setUniform1i("show_bone_weight_id", human_with_skeleton.show_bone_weight_id);

        gizmo_shader.apply();
        gizmo_shader.setUniform1iv("frame_ids", 3, frame_ids.data());
        gizmo_shader.setUniform1f("left_weight", weight_left_frame);
        gizmo_shader.setUniform1f("right_weight", weight_right_frame);
        // auto track_tex_id = std::vector<int>{};
        // for (auto id = 0; id < human_with_skeleton.tracks.size(); id++) {
        //     track_tex_id.emplace_back(id + 2);
        // }
        gizmo_shader.setUniform1iv("bone_current_poses", human_with_skeleton.tracks.size(), track_tex_id.data());
        gizmo_shader.setUniform1i("blend_anim_num", 3);
        gizmo_shader.setUniform1fv("blend_weights", 3, blend_weights.data());
        gizmo_shader.setUniform4fv("gizmo_color", bone_gizmo_color);
        gizmo_shader.setUniform1f("gizmo_scale", gizmo_model.scale);

        shader.apply();
        human_with_skeleton.draw();

        if (show_bone_gizmo && gizmo_model.scale > 0.0f) {
            glDisable(GL_DEPTH_TEST);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
            gizmo_shader.apply();
            for (auto bone_gz_id = 0; bone_gz_id < human_with_skeleton.bones.size(); bone_gz_id++) {
                gizmo_shader.setUniform1i("bone_id", bone_gz_id);
                gizmo_shader.setUniform1i("show_bone_weight_id", human_with_skeleton.show_bone_weight_id);
                gizmo_model.draw();
            }
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
        }

        if ( weight_right_frame >= 1.0f) {
            for (auto& frame_id: frame_ids) {
                frame_id++;
            }

            // time = 0.0f;
            weight_right_frame = 0.0f;
            // last_clock = ;
            // fps = 0;
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

            auto draw_imgui = [&]() -> void {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                auto anim_tool_active{true};
                ImGui::Begin("Animation Tools", &anim_tool_active, ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar);
                ImGui::TextColored(ImVec4(1, 0, 0, 1), std::format("fps {:d}", final_fps).c_str());
                if (human_with_skeleton.import_animation) {
                    ImGui::SliderFloat("speed", &human_with_skeleton.speed, 0.0f, 2.0f);
                    ImGui::SliderFloat("scale", &human_with_skeleton.scale, 0.0f, human_with_skeleton_config_scale * 2.0f);
                    if (!show_blend_space) {
                        ImGui::Text("%d - %s", human_with_skeleton.play_anim_track, human_with_skeleton.tracks[human_with_skeleton.play_anim_track].track_name.c_str());
                        ImGui::SliderInt("track", &human_with_skeleton.play_anim_track, 0, human_with_skeleton.tracks.size() - 1);
                    }

                    if (human_with_skeleton.show_bone_weight_id >= 0)
                        ImGui::Text("%d - %s", human_with_skeleton.show_bone_weight_id, human_with_skeleton.bones[human_with_skeleton.show_bone_weight_id].name.c_str());
                    else
                        ImGui::Text("Disable bone weight visualize");
                    ImGui::SliderInt("bone", &human_with_skeleton.show_bone_weight_id, -1, human_with_skeleton.bones.size() - 1);

                    // ImGui::Text("Blend Space");
                    ImGui::Checkbox("show blend space", &show_blend_space);
                    // ImGui::InvisibleButton("layout", ImVec2(100, 100), 0);
                    if (show_blend_space) {
                        ImGui::SliderFloat("test weight", &blend_weights[0], 0.0f, 1.0f);
                        blend_weights[1] = 1.0f - blend_weights[0];
                        auto iID = ImGui::GetID("layout");
                        ImGui::PushID(iID);
                        auto component_width = 0.7f * (ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
                        auto componect_pos = ImGui::GetCursorScreenPos();
                        auto component_rect = ImRect(componect_pos, ImVec2(component_width, component_width) + componect_pos);
                        static auto cur_offset_in_rect = ImVec2(0, 0);

                        ImGui::GetForegroundDrawList()->AddRect(component_rect.Min, component_rect.Max, IM_COL32(0, 0, 255, 255), 0.0f, 0, 3.0f);

                        if (ImGui::IsMouseHoveringRect(component_rect.Min, component_rect.Max) && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
                            auto cur_pos = ImGui::GetMousePos();
                            cur_offset_in_rect = (cur_pos - component_rect.Min) / component_width;
                        }
                        auto cur_pos_ref = cur_offset_in_rect * component_width + component_rect.Min;

                        ImGui::GetForegroundDrawList()->AddLine(ImVec2(cur_pos_ref.x, component_rect.Min.y), ImVec2(cur_pos_ref.x, component_rect.Max.y), IM_COL32(100, 100, 100, 255), 3.0f);
                        ImGui::GetForegroundDrawList()->AddLine(ImVec2(component_rect.Min.x, cur_pos_ref.y), ImVec2(component_rect.Max.x, cur_pos_ref.y), IM_COL32(100, 100, 100, 255), 3.0f);
                        ImGui::GetForegroundDrawList()->AddCircleFilled(cur_pos_ref, 3.0f, IM_COL32( 255, 255, 0, 255 ), 0);

                        ImGui::PopID();
                        ImGui::Dummy(ImVec2(component_width, component_width));
                        // assert(false);
                    }

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
            };
            draw_imgui();
            glfwGetFramebufferSize(render::window::window, &render::window::SCR_WIDTH, &render::window::SCR_HEIGHT);
            projection_matrix = glm::perspectiveFov(glm::radians(60.0f), float(render::window::SCR_WIDTH), float(render::window::SCR_HEIGHT), 0.1f, 10.0f);
            glViewport(0, 0, render::window::SCR_WIDTH, render::window::SCR_HEIGHT);

            display();

            // assert(false);

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
