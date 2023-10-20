#include "group-animation.hpp"
#include <format>

namespace Group_Animation
{
    auto Boid::strategy() -> void
    {

    }

    auto Boid::update(std::vector<Boid>& boids, float delta_time) -> void
    {
        constexpr float min_distance{1.0f};
        constexpr float avoid_factor{0.01f};
        constexpr float center_factor{0.01f};
        constexpr float align_factor{0.01f};
        constexpr float visual_range{1.0f};
        glm::vec3 move{};
        glm::vec3 center{};
        glm::vec3 align_vec{};
        auto neighbor_num{0};
        for (auto& boid: boids) {
            auto dis = glm::distance(position, boid.position);
            if (dis < min_distance) {
                move += position - boid.position;
            }
            if (dis < visual_range) {
                center += boid.position;
                align_vec += boid.velocity;
                neighbor_num++;
            }
        }

        if (neighbor_num > 0) {
            center /= float(neighbor_num);
            align_vec /= float(neighbor_num);
            velocity += (center - position) * center_factor;
            velocity += (align_vec - velocity) * align_factor;
        }

        velocity += move * avoid_factor;
        velocity = glm::normalize(velocity);


        if (position.x < -1) {
            velocity.x += 0.1f;
        }
        if (position.x > 1) {
            velocity.x -= 0.1f;
        }
        if (position.y < -1) {
            velocity.y += 0.1f;
        }
        if (position.y > 1) {
            velocity.y -= 0.1f;
        }
        if (position.z < -1) {
            velocity.z += 0.1f;
        }
        if (position.z > 1) {
            velocity.z -= 0.1f;
        }
        rotation = glm::quatLookAt(velocity, glm::vec3{0.0f, 1.0f, 0.0f});
        position += velocity * delta_time;
        // std::cout << std::format("{:.2f} {:.2f} {:.2f}\n", position.x, position.y, position.z);
    }

    auto Boid::get_affine_matrix() -> glm::mat4x4
    {
        return glm::translate(glm::mat4x4(1.0f), position) * glm::toMat4(rotation) * glm::scale(glm::mat4x4(1.0f), glm::vec3(0.002f));
    }

    auto Flock::init() -> void
    {
        boid_model.load_with_config("asset/boid_config.json");
        for (auto i = 0; i < 10; i++) {
            boids.emplace_back(glm::vec3{float(rand())/float(RAND_MAX), float(rand())/float(RAND_MAX), float(rand())/float(RAND_MAX)});
        }
    }

    auto Flock::update(float delta_time) -> void
    {
        for (auto& boid: boids) {
            boid.update(boids, delta_time);
        }
    }

    auto Flock::draw(render::Shader& shader) -> void
    {
        shader.apply();
        for (auto& boid: boids) {
            auto model_matrix = boid.get_affine_matrix();
            shader.setUniformMatrix4fv("world", model_matrix);
            shader.setUniform1i("import_animation", boid_model.import_animation);
            boid_model.draw();
        }
    }
} // namespace Group_Animation

