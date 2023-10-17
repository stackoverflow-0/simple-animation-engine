#include "animation.hpp"

#include "render/cmake-source-dir.hpp"

#include <assert.h>

#include <nlohmann/json.hpp>
#include <fstream>


namespace Blendspace2D
{
    auto Triangle::get_weight(glm::vec2 p) -> glm::vec3 {
        auto weight_x = (- (p.x - p1.position.x) * (p2.position.y - p1.position.y) + (p.y - p1.position.y) * (p2.position.x - p1.position.x))
                        / (- (p0.position.x - p1.position.x) * (p2.position.y - p1.position.y) + (p0.position.y - p1.position.y) * (p2.position.x - p1.position.x));

        auto weight_y = (- (p.x - p2.position.x) * (p0.position.y - p2.position.y) + (p.y - p2.position.y) * (p0.position.x - p2.position.x))
                        / (- (p1.position.x - p2.position.x) * (p0.position.y - p2.position.y) + (p1.position.y - p2.position.y) * (p0.position.x - p2.position.x));

        auto weight_z = 1.0f - weight_x - weight_y;

        return glm::vec3{weight_x, weight_y, weight_z};
    }

    auto Triangle::get_circumscribed_circle() -> glm::vec3 {
        auto a = glm::distance(p0.position, p1.position);
        auto b = glm::distance(p2.position, p0.position);
        auto c = glm::distance(p2.position, p1.position);
        auto radius = (a * b * c) / glm::sqrt((a + b + c) * (b + c - a)*(c + a - b) * ( a + b - c));
        auto d = 2 * (p0.position.x * (p1.position.y - p2.position.y) + p1.position.x * (p2.position.y - p0.position.y) + p2.position.x * (p0.position.y - p1.position.y));
        auto xp = ((p0.position.x * p0.position.x + p0.position.y * p0.position.y) * (p1.position.y - p2.position.y) + (p1.position.x * p1.position.x + p1.position.y * p1.position.y) * (p2.position.y - p0.position.y) + (p2.position.x * p2.position.x + p2.position.y * p2.position.y) * (p0.position.y - p1.position.y)) / d;
        auto yp = ((p0.position.x * p0.position.x + p0.position.y * p0.position.y) * (p2.position.x - p1.position.x) + (p1.position.x * p1.position.x + p1.position.y * p1.position.y) * (p0.position.x - p2.position.x) + (p2.position.x * p2.position.x + p2.position.y * p2.position.y) * (p1.position.x - p0.position.x)) / d;
        return glm::vec3{xp, yp, radius};
    }

    auto Triangle::share_edge_with(Triangle& t) -> bool {
        auto cnt{0};
        if (in_triangle(t.p0))
            cnt++;
        if (in_triangle(t.p1))
            cnt++;
        if (in_triangle(t.p2))
            cnt++;
        return cnt >= 2 ;
    }

    auto Triangle::in_triangle(Node& n) -> bool {
        return n == p0 || n == p1 || n == p2;
    }

    auto Blend_Space_2D::init(assimp_model::Model& model, const std::string path) -> void {
        frame_ids.resize(model.tracks.size(), 0);
        // for (auto& t: model.tracks) {
        //     track_len.emplace_back(t.duration);
        // }

        blend_weight.resize(3, 0);
        track_ids.resize(3, 0);

        std::ifstream config_fs(ROOT_DIR + path);

        auto config = nlohmann::json::parse(config_fs, nullptr, true, true);

        std::vector<Node> tmp_nodes{};

        auto max_x{-INFINITY};
        auto max_y{-INFINITY};
        auto min_x{INFINITY};
        auto min_y{INFINITY};

        auto& nodes = config.find("node").value();
        for (auto& node: nodes) {
            float x = node.find("x").value();
            float y = node.find("y").value();
            int track_id = node.find("anim_id").value();
            tmp_nodes.emplace_back(glm::vec2{x, y}, track_id);
            if (x > max_x) {
                max_x = x;
            }
            if (x < min_x) {
                min_x = x;
            }
            if (y > max_y) {
                max_y = y;
            }
            if (y < min_y) {
                min_y = y;
            }
        }

        auto delaunay_triangulation = [&]() -> void {
            // auto p_max_max = Node{glm::vec2{max_x, max_y}, -1};
            // auto p_max_min = Node{glm::vec2{max_x, min_y}, -1};
            // auto p_min_max = Node{glm::vec2{min_x, max_y}, -1};
            // auto p_min_min = Node{glm::vec2{min_x, min_y}, -1};

            auto p_max_max = Node{glm::vec2{1, 1}, -1};
            auto p_max_min = Node{glm::vec2{1, -1}, -1};
            auto p_min_max = Node{glm::vec2{-1, 1}, -1};
            auto p_min_min = Node{glm::vec2{-1, -1}, -1};

            triangles.emplace_back(p_max_max, p_min_max, p_min_min);
            triangles.emplace_back(p_max_max, p_max_min, p_min_min);

            for (auto& n: tmp_nodes) {
                auto star_poly = std::vector<Node>{};
                for (auto it = triangles.begin(); it != triangles.end();) {
                    auto& triangle = *it;
                    auto info = triangle.get_circumscribed_circle();
                    auto center = glm::vec2{info.x, info.y};
                    if (glm::distance(n.position, center) <= info.z - 0.0001) {
                        auto p0_exist{false};
                        auto p1_exist{false};
                        auto p2_exist{false};
                        for (auto& p: star_poly) {
                            if (p == triangle.p0) {
                                p0_exist = true;
                            }
                            if (p == triangle.p1) {
                                p1_exist = true;
                            }
                            if (p == triangle.p2) {
                                p2_exist = true;
                            }
                        }

                        if (!p0_exist)
                            star_poly.emplace_back(triangle.p0);
                        if (!p1_exist)
                            star_poly.emplace_back(triangle.p1);
                        if (!p2_exist)
                            star_poly.emplace_back(triangle.p2);

                        it = triangles.erase(it);
                    } else {
                        it++;
                    }
                }

                auto angle_greater = [&](auto& p1, auto& p2) -> bool {
                    return glm::orientedAngle(p1.position - n.position, glm::vec2{}) > glm::orientedAngle(p2.position - n.position, glm::vec2{});
                };

                if (!star_poly.empty()) {
                    std::sort(
                        star_poly.begin(),
                        star_poly.end(),
                        angle_greater
                    );
                    for (auto i = 0; i < star_poly.size() - 1; i++) {
                        triangles.emplace_back(n, star_poly[i], star_poly[i + 1]);
                    }
                    triangles.emplace_back(n, star_poly[star_poly.size() - 1], star_poly[0]);
                }
            }
            auto triangles_to_be_remove = std::vector<Triangle>{};
            for (auto it = triangles.begin(); it != triangles.end(); ) {
                auto& t = *it;
                if (t.p0.track_id == -1 || t.p1.track_id == -1 || t.p2.track_id == -1) {
                    triangles_to_be_remove.emplace_back(t);
                    it = triangles.erase(it);
                } else {
                    it++;
                }
            }

            for (auto it = triangles_to_be_remove.begin(); it != triangles_to_be_remove.end(); it++) {
                auto& t = *it;
                for (auto it_ = it; it_ != triangles_to_be_remove.end(); it_++) {
                    auto t_ = *it_;
                    if (t.share_edge_with(t_)) {
                        std::unordered_set<Node, Node_Hash> uset;
                        std::vector<Node> uvec{};
                        if (t.p0.track_id != -1)
                            uset.emplace(t.p0);
                        if (t.p1.track_id != -1)
                            uset.emplace(t.p1);
                        if (t.p2.track_id != -1)
                            uset.emplace(t.p2);
                        if (t_.p0.track_id != -1)
                            uset.emplace(t_.p0);
                        if (t_.p1.track_id != -1)
                            uset.emplace(t_.p1);
                        if (t_.p2.track_id != -1)
                            uset.emplace(t_.p2);
                        for (auto& p: uset) {
                            uvec.emplace_back(p);
                        }
                        if (uvec.size() == 3)
                            triangles.emplace_back(uvec[0], uvec[1], uvec[2]);
                    }
                }
            }
        };

        delaunay_triangulation();
        std::cout << "mk\n";
    }

    auto Blend_Space_2D::update(assimp_model::Model& model, glm::vec2 p, float& left_weight, float& right_weight) -> void {
        position = p;
        if (right_weight >= 1.0f) {
            for (auto i = 0; i < frame_ids.size(); i++) {
                frame_ids[i]++;
                if (frame_ids[i] >= model.tracks[i].duration - 1) {
                    frame_ids[i] = 0;
                }
            }
            left_weight = 1.0f;
            right_weight = 0.0f;
        }

        for (auto& triangle: triangles) {
            auto w = triangle.get_weight(position);
            if (w.x >= 0.0f && w.y >= 0.0f && w.z >= 0.0f) {
                blend_weight[0] = w.x;
                blend_weight[1] = w.y;
                blend_weight[2] = w.z;
                track_ids[0] = triangle.p0.track_id;
                track_ids[1] = triangle.p1.track_id;
                track_ids[2] = triangle.p2.track_id;
                // in_blend_space = true;
                break;
            }
        }

        model.blend_tracks(frame_ids, track_ids, left_weight, right_weight, blend_weight);
        model.bind_textures();
    }
} // namespace Blendspace2D

