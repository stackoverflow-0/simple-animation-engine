#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
    #define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <imgui_internal.h>

#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "mesh.hpp"

namespace Blendspace2D
{
    struct Node final
    {
        glm::vec2 position{};
        int track_id{};
        bool operator == (const Node& t) const {
            return t.track_id == track_id && t.position == position;
        }
    };

    struct Node_Hash final {
        size_t operator()(const Node& t) const
        {
            std::hash<float> hash_float;
            return hash_float(t.position.x) & hash_float(t.position.y) & hash_float(t.track_id);
        }
    };

    struct Triangle final
    {
        Node p0{};
        Node p1{};
        Node p2{};

        auto get_weight(glm::vec2 p) -> glm::vec3;

        auto inside_triangle(glm::vec2& p) -> bool;
        // glm::vec3 z -> radius , x / y = center coordinate x / y
        auto get_circumscribed_circle() -> glm::vec3;

        auto share_edge_with(Triangle& t) -> bool;

        auto in_triangle(Node& n) -> bool;

        auto is_convex_with(Triangle& t) -> bool;


    };
    struct Blend_Space_2D final
    {
        glm::vec2 position{};
        std::vector<int> frame_ids{};
        // std::vector<int> track_len{};
        std::vector<Triangle> triangles{};
        std::vector<float> blend_weight{};
        std::vector<int> track_ids{};

        // bool in_blend_space{true};

        // std::unordered_map<glm::vec2, int> point_to_track;
        auto init(assimp_model::Model& model, const std::string path) -> void;

        auto update(assimp_model::Model& model, glm::vec2 p, float& left_weight, float& right_weight) -> void;
    };
} // namespace Blendspace2D
