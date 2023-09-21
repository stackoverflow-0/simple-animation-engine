#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <GL/glew.h>
#include <vector>
#include <iostream>
#include <unordered_map>

#include "render/cmake-source-dir.hpp"

namespace assimp_model
{
    struct driven_bone final
    {
        unsigned int driven_bone_id{};
        float driven_bone_weight{};
    };
    struct Vertex final
    {
        glm::vec3 position{};
        glm::vec3 normal{};
        glm::vec2 texcoords{};
        glm::vec2 bone_weight_offset{};
    };

    struct Bone final
    {
        glm::mat4x4 bind_pose_local{};
        int parent_id{};
        std::string name{};
        std::vector<int> child_id{};
    };

    struct Channel final
    {
        std::vector<glm::quat> rotations{};
        std::vector<float> times{};
    };

    struct Track final
    {
        std::string track_name{};
        float duration{};
        float frame_per_second{1};
        std::vector<Channel> channels{};
    };

    struct Mesh final
    {
        unsigned int vao{};
        unsigned int vbo{};
        unsigned int ebo{};
        unsigned int bone_weight_texture{};
        std::vector<Vertex> vertices{};
        std::vector<unsigned int> indices{};
        std::vector<glm::vec2> bone_id_and_weight{};

        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
        {
            this->vertices = vertices;
            this->indices = indices;
            // now that we have all the required data, set the vertex buffers and its attribute pointers.
            // setup_mesh();
        }

        auto draw() noexcept -> void
        {
            // draw mesh
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        auto append_mesh(std::vector<Vertex>& append_vertices, std::vector<unsigned int>& append_indices, std::vector<glm::vec2>& append_driven_bone_offset, std::vector<std::vector<driven_bone>>& append_driven_bone_and_weight) noexcept -> void;

        auto setup_mesh() noexcept -> void;
    };

    struct Model final
    {
        Mesh uniform_mesh = Mesh({}, {});
        unsigned int bind_pose_texture{};
        std::vector<Bone> bones{};
        std::unordered_map<std::string, unsigned int> bone_name_to_id{};
        std::vector<Track> tracks{};
        // std::vector<glm::mat4x4> bind_pose_local_with_skinning{};
        std::string directory;

        auto draw() noexcept -> void
        {
            uniform_mesh.draw();
        }

        auto load_model(std::string const path) noexcept -> bool;

        auto processNode(aiNode *node, const aiScene *scene) -> void;

        auto processMesh(aiMesh *mesh, const aiScene *scene) -> void;

        auto create_bind_pose_matrix_texure() -> void;

        auto setup_model() -> void
        {
            uniform_mesh.setup_mesh();
            create_bind_pose_matrix_texure();
        }
    };
} // namespace mesh
