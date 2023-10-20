#include "render/mesh.hpp"
#include "render/render.hpp"
#include "render/animation.hpp"
#include "render/group-animation.hpp"
#include <stdio.h>
#include <assert.h>
#include <thread>
#include <format>

#include <chrono>

int main()
{
    // std::cout << sizeof(glm::dualquat) / sizeof(float) << std::endl;
    auto setup_status = render::setup_glfw3_and_imgui();

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

    assimp_model::Model gizmo_model{};
    gizmo_model.load_with_config("asset/gizmo_config.json");

    auto world_matrix = glm::mat4(1.0f);

    auto projection_matrix = glm::perspectiveFov(glm::radians(60.0f), float(1024), float(768), 0.1f, 20.0f);

    auto time{0.0f};
    auto last_clock = std::chrono::high_resolution_clock().now();

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

    auto slider2d_pos = ImVec2(0, 0);

    Blendspace2D::Blend_Space_2D blend_space{};
    blend_space.init(human_with_skeleton, "asset/blend-space.json");

    Group_Animation::Flock flock{};
    flock.init();

    auto display = [&]()
    {
        auto view_matrix  = glm::lookAt(render::window::cam_position, render::window::cam_look_at, render::window::cam_up);
        auto world_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(-0.0f), glm::vec3(1, 0, 0));
        world_matrix = glm::rotate(world_matrix, glm::radians(0.0f), glm::vec3(0, 0, 1));
        world_matrix = glm::translate(world_matrix, glm::vec3(0, -0.4, -0.1));
        world_matrix = glm::scale(world_matrix, glm::vec3(human_with_skeleton.scale));

        shader.apply();
        shader.setUniformMatrix4fv("world", world_matrix);
        shader.setUniformMatrix4fv("viewProj", projection_matrix * view_matrix);
        shader.setUniform3fv("cam_pos", render::window::cam_position);
//
        flock.update(0.01f);
        // flock.draw(shader);
        shader.apply();
        for (auto& boid: flock.boids) {
            auto model_matrix = boid.get_affine_matrix() * glm::scale(glm::mat4x4(1.0f), glm::vec3(flock.boid_model.scale));
            shader.setUniformMatrix4fv("world", model_matrix);
            shader.setUniform1i("import_animation", 0);
            flock.boid_model.draw();
        }

        shader.apply();
        shader.setUniformMatrix4fv("world", world_matrix);
        shader.setUniformMatrix4fv("viewProj", projection_matrix * view_matrix);

        gizmo_shader.apply();
        gizmo_shader.setUniformMatrix4fv("world", world_matrix);
        gizmo_shader.setUniformMatrix4fv("viewProj", projection_matrix * view_matrix);
        auto current_clock = std::chrono::high_resolution_clock().now();

        auto update_time_and_logic = [&]() -> void {
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
                speed = human_with_skeleton.speed;
            }

            auto& track = human_with_skeleton.tracks[human_with_skeleton.play_anim_track];

            auto frame_time{1.0f / speed / track.frame_per_second};

            if (human_with_skeleton.speed > 0.0f) {
                weight_right_frame += delta_frame_time / frame_time;
                weight_left_frame = 1.0f - weight_right_frame;
            }
        };

        update_time_and_logic();

        auto update_animation = [&]() -> void {
            blend_space.update(human_with_skeleton, glm::vec2(slider2d_pos.x, slider2d_pos.y), weight_left_frame, weight_right_frame);
        };

        update_animation();

        shader.apply();
        shader.setUniform1b("import_animation", human_with_skeleton.import_animation);
        shader.setUniform1i("bone_current_pose", 2);
        shader.setUniform1i("show_bone_weight_id", human_with_skeleton.show_bone_weight_id);

        gizmo_shader.apply();
        gizmo_shader.setUniform1i("bone_current_pose", 2);
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
    };

    auto run = [&]()
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_MULTISAMPLE);
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
                        // ImGui::Text("%d - %s", human_with_skeleton.play_anim_track, human_with_skeleton.tracks[human_with_skeleton.play_anim_track].track_name.c_str());
                        // ImGui::SliderInt("track", &human_with_skeleton.play_anim_track, 0, human_with_skeleton.tracks.size() - 1);
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
                        // ImGui::SliderFloat("test weight", &blend_weights[0], 0.0f, 1.0f);
                        // blend_weights[1] = 1.0f - blend_weights[0];
                        auto iID = ImGui::GetID("layout");
                        ImGui::PushID(iID);
                        auto component_width = 0.7f * (ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
                        auto componect_pos = ImGui::GetCursorScreenPos();
                        auto component_rect = ImRect(componect_pos, ImVec2(component_width, component_width) + componect_pos);
                        ImGui::GetForegroundDrawList()->AddRect(component_rect.Min, component_rect.Max, IM_COL32(0, 0, 255, 255), 0.0f, 0, 3.0f);

                        for (auto& t: blend_space.triangles) {
                            auto p0 = (ImVec2(t.p0.position.x, t.p0.position.y) / 2.0f + ImVec2(0.5f, 0.5f)) * component_width;
                            p0.y = - p0.y + component_width;
                            p0 += component_rect.Min;
                            auto p1 = (ImVec2(t.p1.position.x, t.p1.position.y) / 2.0f + ImVec2(0.5f, 0.5f)) * component_width;
                            p1.y = - p1.y + component_width;
                            p1 += component_rect.Min;
                            auto p2 = (ImVec2(t.p2.position.x, t.p2.position.y) / 2.0f + ImVec2(0.5f, 0.5f)) * component_width;
                            p2.y = - p2.y + component_width;
                            p2 += component_rect.Min;

                            ImGui::GetForegroundDrawList()->AddLine(p0, p1, IM_COL32(255, 255, 255, 255), 3.0f);
                            ImGui::GetForegroundDrawList()->AddLine(p1, p2, IM_COL32(255, 255, 255, 255), 3.0f);
                            ImGui::GetForegroundDrawList()->AddLine(p2, p0, IM_COL32(255, 255, 255, 255), 3.0f);
                            ImGui::GetForegroundDrawList()->AddCircleFilled(p0, 1.5f, IM_COL32( 0, 255, 255, 255 ), 0);
                            ImGui::GetForegroundDrawList()->AddCircleFilled(p1, 1.5f, IM_COL32( 0, 255, 255, 255 ), 0);
                            ImGui::GetForegroundDrawList()->AddCircleFilled(p2, 1.5f, IM_COL32( 0, 255, 255, 255 ), 0);
                        }

                        if (ImGui::IsMouseHoveringRect(component_rect.Min, component_rect.Max) && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
                            auto cur_pos = ImGui::GetMousePos();
                            auto tmp_slider2d_pos = (cur_pos - component_rect.GetCenter()) / component_width * 2.0f;
                            tmp_slider2d_pos.y = - tmp_slider2d_pos.y;
                            // blend_space.update(human_with_skeleton, glm::vec2(tmp_slider2d_pos.x, tmp_slider2d_pos.y), weight_left_frame, weight_right_frame);
                            slider2d_pos = tmp_slider2d_pos;
                        }
                        auto cur_pos_ref = (slider2d_pos / 2.0f + ImVec2(0.5f, 0.5f)) * component_width;
                        cur_pos_ref.y = - cur_pos_ref.y + component_width;
                        cur_pos_ref += component_rect.Min;

                        auto bs_pos_ref = (ImVec2(blend_space.position.x, blend_space.position.y) / 2.0f + ImVec2(0.5f, 0.5f)) * component_width;
                        bs_pos_ref.y = - bs_pos_ref.y + component_width;
                        bs_pos_ref += component_rect.Min;

                        ImGui::GetForegroundDrawList()->AddLine(ImVec2(cur_pos_ref.x, component_rect.Min.y), ImVec2(cur_pos_ref.x, component_rect.Max.y), IM_COL32(100, 100, 100, 255), 3.0f);
                        ImGui::GetForegroundDrawList()->AddLine(ImVec2(component_rect.Min.x, cur_pos_ref.y), ImVec2(component_rect.Max.x, cur_pos_ref.y), IM_COL32(100, 100, 100, 255), 3.0f);

                        ImGui::GetForegroundDrawList()->AddCircleFilled(cur_pos_ref, 3.0f, IM_COL32( 255, 255, 0, 255 ), 0);
                        ImGui::GetForegroundDrawList()->AddCircleFilled(bs_pos_ref, 3.0f, IM_COL32( 255, 255, 0, 255 ), 0);

                        ImGui::PopID();
                        ImGui::Dummy(ImVec2(component_width, component_width));
                        // assert(false);
                        ImGui::Text("blend position x %.2f - y %.2f\n", blend_space.position.x, blend_space.position.y);
                        ImGui::Text(
                            "blend weight\nanim%d - %.2f\nanim%d - %.2f\nanim%d - %.2f\n",
                            blend_space.track_ids[0], blend_space.blend_weight[0],
                            blend_space.track_ids[1], blend_space.blend_weight[1],
                            blend_space.track_ids[2], blend_space.blend_weight[2]
                        );
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

            auto update_camera = [&]() -> void  {
                glfwGetFramebufferSize(render::window::window, &render::window::SCR_WIDTH, &render::window::SCR_HEIGHT);
                projection_matrix = glm::perspectiveFov(glm::radians(60.0f), float(render::window::SCR_WIDTH), float(render::window::SCR_HEIGHT), 0.1f, 10.0f);
                glViewport(0, 0, render::window::SCR_WIDTH, render::window::SCR_HEIGHT);
            };

            draw_imgui();
            update_camera();
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
