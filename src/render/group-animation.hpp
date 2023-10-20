#pragma once

#include "animation.hpp"
#include "mesh.hpp"
#include "render.hpp"

#include <glm/glm.hpp>

namespace Group_Animation
{
    struct Agent final
    {

    };

    struct Boid final
    {
        glm::vec3 position{};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 velocity{0.0f, 0.0f, 1.0f};
        
        auto strategy() -> void;

        auto get_affine_matrix() -> glm::mat4x4;

        auto update(std::vector<Boid>& boids, float delta_time) -> void;
    };

    struct Flock final
    {
        std::vector<Boid> boids{};

        assimp_model::Model boid_model;

        render::Shader compute_shader;

        auto init() -> void;

        auto update(float delta_time) -> void;

        auto draw(render::Shader& shader) -> void;
    };
} // namespace Group_Animation

