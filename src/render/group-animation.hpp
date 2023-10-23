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

    struct Flock;

    struct Boid final
    {
        glm::vec4 position{};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec4 velocity{0.0f, 0.0f, 1.0f, 0.0f};
        
        auto strategy() -> void;

        auto get_affine_matrix() -> glm::mat4x4;

        auto update(Flock& flock, float delta_time) -> void;
    };

    struct Flock final
    {
        std::vector<Boid> boids{};

        assimp_model::Model boid_model;

        render::Shader compute_shader;

        unsigned int boid_buffer{};

        bool enable_gpu{true};

        int boid_num{10};

        Boid* mappedData{nullptr};

        float min_distance{0.3f};
        float avoid_factor{0.05f};
        float center_factor{0.01f};
        float align_factor{0.01f};
        float visual_range{0.4f};

        auto init(const std::string boid_config_path, const std::string flock_config_path) -> void;

        auto update(float delta_time) -> void;

        auto draw(render::Shader& shader) -> void;
    };
} // namespace Group_Animation

