#include "mesh.hpp"
#include <format>
#include <queue>

namespace assimp_model
{
    constexpr int animation_index_texture_width = 1024;

    auto Mesh::append_mesh(std::vector<Vertex>& append_vertices, std::vector<unsigned int>& append_indices, std::vector<driven_bone>& append_bone_weight) noexcept -> void
    {
        auto indices_offset = vertices.size();
        int i{0};
        for (auto v: append_vertices) {
            v.driven_bone_id = append_bone_weight[i].driven_bone_id;
            v.driven_bone_weight = append_bone_weight[i].driven_bone_weight;
            // v.b_and_w = glm::vec4(0,0.9,0,0);
            i++;
            vertices.emplace_back(v);
        }
        // for (auto bw: append_bone_weight) {
        //     driven_bone_and_weight.emplace_back(bw);
        // }
        for (auto i: append_indices) {
            indices.emplace_back(indices_offset + i);
        }
    }

    auto Mesh::setup_mesh() noexcept -> void
    {
        // create buffers/arrays
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texcoords));
        // vertex animation texture coords
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, driven_bone_id));

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, driven_bone_weight));

        glBindVertexArray(0);
    }

    auto Model::load_model(std::string const path) noexcept -> bool
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ROOT_DIR + path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return false;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        auto processSkeleton = [&]()-> void {
            if (scene->HasAnimations()) {
            auto anim_num = scene->mNumAnimations;
            tracks.resize(anim_num);

            std::cout  << std::format("anim count {:d}\n", anim_num);

            for (auto anim_id = 0; anim_id < anim_num; anim_id++) {

                auto& track = tracks[anim_id];

                auto anim = scene->mAnimations[anim_id];
                auto anim_channel_num = anim->mNumChannels;
                std::cout  << std::format("anim track name {:s}\n", anim->mName.C_Str());

                track.duration = anim->mDuration;
                track.frame_per_second = anim->mTicksPerSecond;
                track.channels.resize(anim_channel_num);

                for (auto channel_id = 0; channel_id < anim_channel_num; channel_id++) {
                    auto channel_node = anim->mChannels[channel_id];
                    auto& channel = track.channels[channel_id];
                    if (bone_name_to_id.find(channel_node->mNodeName.C_Str()) == bone_name_to_id.end())
                        bone_name_to_id.emplace(channel_node->mNodeName.C_Str(), bone_name_to_id.size());
                    // std::cout << std::format("bone name to id : {:s}\n", channel_node->mNodeName.C_Str());

                    channel.rotations.resize(channel_node->mNumRotationKeys);
                    channel.times.resize(channel_node->mNumRotationKeys);

                    for (auto key_id = 0; key_id < channel_node->mNumRotationKeys; key_id++) {
                        auto& rot = channel_node->mRotationKeys[key_id].mValue;
                        channel.rotations[key_id] = glm::quat(rot.w, rot.x, rot.y, rot.z);
                        channel.times[key_id] = channel_node->mRotationKeys[key_id].mTime;
                    }
                    
                }
            }
            }

            auto bone_root = scene->mRootNode;

            auto walk_bone_tree = [&]() -> void {
                bones.resize(bone_name_to_id.size());
                std::queue<aiNode*> bone_to_be_walk({bone_root});

                while (!bone_to_be_walk.empty()) {
                    auto bone_node = bone_to_be_walk.front();
                    bone_to_be_walk.pop();
                    std::string bone_name = bone_node->mName.C_Str();
                    // std::cout << std::format("bone name : {:s}\n", bone_name);
                    std::string parent_name{};
                    int parent_id{-1};
                    std::vector<int> child_id{};

                    if (bone_node->mParent != nullptr) {
                        parent_name = bone_node->mParent->mName.C_Str();
                        if (bone_name_to_id.find(parent_name) != bone_name_to_id.end()) {
                            parent_id = bone_name_to_id.at(parent_name);
                        }
                    }

                    for (auto bone_child_i = 0; bone_child_i < bone_node->mNumChildren; bone_child_i++) {
                        bone_to_be_walk.push(bone_node->mChildren[bone_child_i]);
                        // bone_name_to_id.at(bone_node->mChildren[bone_child_i]->mName.C_Str());
                        std::string child_name = bone_node->mChildren[bone_child_i]->mName.C_Str();
                        if (bone_name_to_id.find(child_name) != bone_name_to_id.end())
                            child_id.emplace_back(bone_name_to_id.at(child_name));
                    }
                    auto& trans_mat = bone_node->mTransformation;
                    glm::mat4x4 transformation{
                        trans_mat.a1, trans_mat.a2, trans_mat.a3, trans_mat.a4,
                        trans_mat.b1, trans_mat.b2, trans_mat.b3, trans_mat.b4,
                        trans_mat.c1, trans_mat.c2, trans_mat.c3, trans_mat.c4,
                        trans_mat.d1, trans_mat.d2, trans_mat.d3, trans_mat.d4,
                    };
                    if (bone_name_to_id.find(bone_name) != bone_name_to_id.end())
                        bones[bone_name_to_id.at(bone_name)] = {transformation, parent_id, bone_name, child_id};
                }
            };
            walk_bone_tree();
        };
        processSkeleton();

         // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
        uniform_mesh.setup_mesh();
        return true;
    }

    auto Model::processNode(aiNode *node, const aiScene *scene) -> void
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene.
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene);
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    auto Model::processMesh(aiMesh *mesh, const aiScene *scene) -> void
    {
        // data to fill
        static int vertex_idx = 0;
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // Walk through each of the mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
                              // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.position = vector;
            // normals
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.normal = vector;
            // texture coordinates
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.texcoords = vec;
            }
            else
                vertex.texcoords = glm::vec2(0.0f, 0.0f);
            
            // vertex.b_and_w = vertex_idx;
            vertex_idx++;

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        std::vector<driven_bone> driven_bone_and_weight{};
        driven_bone_and_weight.resize(vertices.size());

        auto& bones = mesh->mBones;
        auto bone_num = mesh->mNumBones;
        for (auto i = 0; i < bone_num; i++) {
            auto& bone = bones[i];
            auto bone_id = bone_name_to_id.at(bone->mName.C_Str());
            auto bone_drive_vert_num = bone->mNumWeights;
            for (auto j = 0; j < bone_drive_vert_num; j++) {
                auto vert_id = bone->mWeights[j].mVertexId;
                auto vert_weight = bone->mWeights[j].mWeight;
                auto& b_and_w = driven_bone_and_weight[vert_id];
                if (vert_weight < 0.1f) {
                    continue;
                }
                if (b_and_w.driven_bone_id[0] == 0) {
                    b_and_w.driven_bone_id[0] = vert_id;
                    b_and_w.driven_bone_weight[0] = vert_weight;
                    b_and_w.driven_bone_weight[1] = 1.0f - b_and_w.driven_bone_weight[0];
                } else if (b_and_w.driven_bone_id[1] == 0) {
                    b_and_w.driven_bone_id[1] = vert_id;
                    b_and_w.driven_bone_weight[1] = vert_weight;
                    b_and_w.driven_bone_weight[2] = 1.0f - b_and_w.driven_bone_weight[0] - - b_and_w.driven_bone_weight[1];
                } else if (b_and_w.driven_bone_id[2] == 0) {
                    b_and_w.driven_bone_id[2] = vert_id;
                    b_and_w.driven_bone_weight[2] = vert_weight;
                    b_and_w.driven_bone_weight[3] =  1.0f - b_and_w.driven_bone_weight[0] - - b_and_w.driven_bone_weight[1] - b_and_w.driven_bone_weight[2];
                } else if (b_and_w.driven_bone_id[3] == 0) {
                    b_and_w.driven_bone_id[3] = vert_id;
                    // b_and_w.driven_bone_weight[3] = vert_weight;
                }
            }
        }

        uniform_mesh.append_mesh(vertices, indices, driven_bone_and_weight);
    }
} // namespace model
