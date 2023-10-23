#include "group-animation.hpp"
#include <nlohmann/json.hpp>
#include <format>
#include <thread>
#include <fstream>

namespace Group_Animation
{
    auto Boid::strategy() -> void
    {

    }

    auto Boid::update(Flock& flock, float delta_time) -> void
    {
        auto nav_point = glm::vec3{};

        glm::vec4 old_vec = velocity;
        glm::vec4 move{};
        glm::vec4 center{};
        glm::vec4 align_vec{};
        auto neighbor_num{0};

        for (auto& boid: flock.boids) {
            auto dis = glm::distance(position, boid.position);
            if (dis < flock.min_distance) {
                move += position - boid.position;
            }
            if (dis < flock.visual_range) {
                center += boid.position;
                align_vec += boid.velocity;
                neighbor_num++;
            }
        }

        if (neighbor_num > 0) {
            center /= float(neighbor_num);
            align_vec /= float(neighbor_num);
            velocity += (center - position) * flock.center_factor;
            velocity += (align_vec - velocity) * flock.align_factor;
        }

        velocity += move * flock.avoid_factor;

        velocity = glm::normalize(velocity);

        // velocity.x += position.x < nav_point.x ? 0.01f : -0.01f;
        // velocity.y += position.y < nav_point.y ? 0.01f : -0.01f;
        // velocity.z += position.z < nav_point.z ? 0.01f : -0.01f;

        // velocity.x += - 0.01f * position.x;
        // velocity.y += - 0.01f * position.y;
        // velocity.z += - 0.01f * position.z;

        velocity.x = position.x > 2.0 ? - glm::abs(velocity.x) : position.x < - 2.0 ? glm::abs(velocity.x) : velocity.x + (position.x < nav_point.x ? 0.01 : -0.01);
        velocity.y = position.y > 2.0 ? - glm::abs(velocity.y) : position.y < - 2.0 ? glm::abs(velocity.y) : velocity.y + (position.y < nav_point.y ? 0.01 : -0.01);
        velocity.z = position.z > 2.0 ? - glm::abs(velocity.z) : position.z < - 2.0 ? glm::abs(velocity.z) : velocity.z + (position.z < nav_point.z ? 0.01 : -0.01);

        velocity = glm::normalize(velocity);

        rotation = glm::rotation(glm::normalize(glm::vec3(old_vec)), glm::normalize(glm::vec3(velocity))) * rotation;
        position += velocity * delta_time;
    }

    auto Boid::get_affine_matrix() -> glm::mat4x4
    {
        return glm::translate(glm::mat4x4(1.0f), glm::vec3(position)) * glm::toMat4(rotation);
    }

    auto Flock::init(const std::string boid_config_path, const std::string flock_config_path) -> void
    {
        boid_model.load_with_config(boid_config_path);
        for (auto i = 0; i < boid_num; i++) {
            boids.emplace_back(glm::vec4{float(rand())/float(RAND_MAX) - 0.5f , float(rand())/float(RAND_MAX) - 0.5f , float(rand())/float(RAND_MAX) - 0.5f, 0.0f});
        }
        compute_shader = render::Shader{{{GL_COMPUTE_SHADER, "asset/shaders/Boid.comp"}}};
        compute_shader.compile();

        glGenBuffers(1, &boid_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, boid_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 10000 * sizeof(Boid), nullptr, GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, boid_buffer);

        std::ifstream cfs(ROOT_DIR + flock_config_path);
        auto flock_cfg = nlohmann::json::parse(cfs, nullptr, true, true);

        min_distance = flock_cfg.find("min_distance").value();
        visual_range = flock_cfg.find("visual_range").value();
        avoid_factor = flock_cfg.find("avoid_factor").value();
        center_factor = flock_cfg.find("center_factor").value();
        align_factor = flock_cfg.find("align_factor").value();

    }

    auto Flock::update(float delta_time) -> void
    {
        static auto gpu_data_dirty{true};

        if (boid_num != boids.size()) {
            if (boids.size() >= boid_num) {
                boids.resize(boid_num);    
            } else {
                while (boids.size() < boid_num)
                    boids.emplace_back(Boid{glm::vec4{float(rand())/float(RAND_MAX) - 0.5f , float(rand())/float(RAND_MAX) - 0.5f , float(rand())/float(RAND_MAX) - 0.5f, 0.0f}});
            }
            gpu_data_dirty = true;
        }
        if (!enable_gpu) {
            gpu_data_dirty = true;
            std::vector<std::thread> thds;
            thds.resize(16);
            auto idx{0};
            for (auto& thd: thds) {
                thd = std::thread([&](int id)-> void { 
                    for (auto i = id; i < boid_num; i += 16) {
                        boids[i].update(*this, delta_time); 
                    }
                }, idx);
                idx++;
            }
            for (auto& thd: thds) {
                if (thd.joinable())
                    thd.join();
            }
        } else {
            if (gpu_data_dirty) {
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, boid_buffer);
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Boid) * boids.size(), boids.data());
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, boid_buffer);
                gpu_data_dirty = false;
            }

            compute_shader.apply();
            compute_shader.setUniform1i("boid_num", boids.size());
            
            compute_shader.apply();
            compute_shader.setUniform1f("min_distance", min_distance);
            compute_shader.setUniform1f("visual_range", visual_range);
            compute_shader.setUniform1f("avoid_factor", avoid_factor);
            compute_shader.setUniform1f("center_factor", center_factor);
            compute_shader.setUniform1f("align_factor", align_factor);
            compute_shader.setUniform1f("delta_time", delta_time);
            glDispatchCompute(16 , 16, 16);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            glFinish();

            mappedData = (Boid*)glMapNamedBufferRange(boid_buffer, 0, sizeof(Boid) * boids.size(), GL_MAP_READ_BIT );

            if (mappedData != nullptr) {
                for (auto i = 0; i < boids.size(); i++) {
                    boids[i] = mappedData[i];
                }
            }

            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }
        

    }

    auto Flock::draw(render::Shader& shader) -> void
    {
        shader.apply();
        for (auto& boid: boids) {
            auto model_matrix = boid.get_affine_matrix() * glm::scale(glm::mat4x4(1.0f), glm::vec3(boid_model.scale));
            shader.setUniformMatrix4fv("world", model_matrix);
            shader.setUniform1i("import_animation", 0);
            boid_model.draw();
        }
    }
} // namespace Group_Animation

