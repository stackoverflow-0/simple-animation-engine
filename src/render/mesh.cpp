#include "mesh.hpp"
#include <format>
#include <queue>

namespace assimp_model
{
    constexpr int animation_index_texture_width = 1024;

    auto Mesh::append_mesh(std::vector<Vertex>& append_vertices, std::vector<unsigned int>& append_indices, std::vector<glm::vec2>& append_driven_bone_offset, std::vector<std::vector<driven_bone>>& append_driven_bone_and_weight) noexcept -> void
    {
        auto indices_offset = vertices.size();
        float driven_bone_offset_offset{0.0};
        if (!vertices.empty())
            driven_bone_offset_offset = vertices.back().bone_weight_offset.x + vertices.back().bone_weight_offset.y;
        int i{0};
        for (auto& v: append_vertices) {
            // v.driven_bone_id = append_bone_weight[i].driven_bone_id;
            // v.driven_bone_weight = append_bone_weight[i].driven_bone_weight;
            // v.b_and_w = glm::vec4(0,0.9,0,0);
            append_driven_bone_offset[i].x += driven_bone_offset_offset;
            v.bone_weight_offset = append_driven_bone_offset[i];
            i++;
            vertices.emplace_back(v);
                // auto sz = vertices.size();
                // if (sz >= 2)
                //     assert(vertices[sz-1].bone_weight_offset.x == vertices[sz-2].bone_weight_offset.x + vertices[sz-2].bone_weight_offset.y);
        }

        for (auto& b_and_w: append_driven_bone_and_weight) {
            for (auto& bw: b_and_w) {
                bone_id_and_weight.emplace_back(float(bw.driven_bone_id), bw.driven_bone_weight);
                // bone_id_and_weight.emplace_back(float(bw.driven_bone_id), 1.0f);
            }
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
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, bone_weight_offset));

        // glEnableVertexAttribArray(4);
        // glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, driven_bone_weight));
        // GLuint blockIndex = glGetUniformBlockIndex(programHandle, "BlobSettings");
        glGenTextures(1, &bone_weight_texture);
        
        glBindTexture(GL_TEXTURE_2D, bone_weight_texture);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, animation_index_texture_width, bone_id_and_weight.size() / 2 / animation_index_texture_width + 1, 0, GL_RGBA, GL_FLOAT, bone_id_and_weight.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bone_weight_texture);

        // for (int i= 0; i< 10; i++) {
        //     float* ptr = reinterpret_cast<float*>(bone_id_and_weight.data());
        //     std::cout << std::format("{:f}", ptr[i]) << std::endl;
        // }

        // glm::vec2 temPvec{114.0f, 514.0f};
        // float* ptr = reinterpret_cast<float*>(&temPvec);
        // std::cout << std::format("{:f}", ptr[0]) << std::endl;
        // std::cout << std::format("{:f}", ptr[1]) << std::endl;
        glBindVertexArray(0);

        auto gen_anim_texture = [&]() -> void {

        };
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
            // break;
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
            // break;
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

        std::vector<std::vector<driven_bone>> driven_bone_and_weight{};
        std::vector<glm::vec2> driven_bone_offset{};
        driven_bone_and_weight.resize(vertices.size());
        driven_bone_offset.resize(vertices.size());

        auto& bones = mesh->mBones;
        auto bone_num = mesh->mNumBones;
        
        for (auto i = 0; i < bone_num; i++) {
            auto& bone = bones[i];
            auto bone_id = bone_name_to_id.at(bone->mName.C_Str());
            auto bone_drive_vert_num = bone->mNumWeights;

            auto& bone_bind_matrix = bone->mOffsetMatrix;
            bone_bind_matrix;
            for (auto j = 0; j < bone_drive_vert_num; j++) {
                auto vert_id = bone->mWeights[j].mVertexId;
                auto vert_weight = bone->mWeights[j].mWeight;
                auto& b_and_w = driven_bone_and_weight[vert_id];
                b_and_w.emplace_back(driven_bone{bone_id, vert_weight});

                // if (b_and_w.driven_bone_id[0] == 0) {
                //     b_and_w.driven_bone_id[0] = vert_id;
                //     b_and_w.driven_bone_weight[0] = vert_weight;
                //     b_and_w.driven_bone_weight[1] = 1.0f - b_and_w.driven_bone_weight[0];
                // } else if (b_and_w.driven_bone_id[1] == 0) {
                //     b_and_w.driven_bone_id[1] = vert_id;
                //     b_and_w.driven_bone_weight[1] = vert_weight;
                //     b_and_w.driven_bone_weight[2] = 1.0f - b_and_w.driven_bone_weight[0] - - b_and_w.driven_bone_weight[1];
                // } else if (b_and_w.driven_bone_id[2] == 0) {
                //     b_and_w.driven_bone_id[2] = vert_id;
                //     b_and_w.driven_bone_weight[2] = vert_weight;
                //     b_and_w.driven_bone_weight[3] =  1.0f - b_and_w.driven_bone_weight[0] - - b_and_w.driven_bone_weight[1] - b_and_w.driven_bone_weight[2];
                // } else if (b_and_w.driven_bone_id[3] == 0) {
                //     b_and_w.driven_bone_id[3] = vert_id;
                //     // b_and_w.driven_bone_weight[3] = vert_weight;
                // }
            }
        }
        auto i{0};
        auto base_offset{0};
        for (auto& b_and_w: driven_bone_and_weight) {
            auto weight_sum{0.0f};
            for (auto& bw: b_and_w) {
                weight_sum += bw.driven_bone_weight;
            }
            assert(std::fabs(weight_sum - 1.0f) < 0.001f);
            driven_bone_offset[i] = glm::vec2{base_offset, b_and_w.size()};
            base_offset += b_and_w.size();
            i++;
        }
        uniform_mesh.append_mesh(vertices, indices, driven_bone_offset, driven_bone_and_weight);
    }
} // namespace model
