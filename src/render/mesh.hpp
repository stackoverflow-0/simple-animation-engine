#pragma once
#include "cmake-source-dir.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <GL/glew.h>
#include <vector>
#include <iostream>

namespace assimp_model
{
    struct Vertex final
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texcoords;
    };

    struct Mesh final
    {
        unsigned int vao{};
        unsigned int vbo{};
        unsigned int ebo{};
        std::vector<Vertex> vertices{};
        std::vector<unsigned int> indices;

        auto draw() noexcept -> void
        {
            // draw mesh
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        auto setup_mesh() noexcept -> void;
    };

    struct Model final
    {
        std::vector<Mesh> sub_meshs{};
        std::string directory;

        auto draw() noexcept -> void
        {
            for (auto &sub_mesh : sub_meshs)
            {
                sub_mesh.draw();
            }
        }

        auto load_model(std::string const path) noexcept -> bool;

        auto processNode(aiNode *node, const aiScene *scene) -> void;

        auto processMesh(aiMesh *mesh, const aiScene *scene) -> Mesh;
    };
} // namespace mesh
