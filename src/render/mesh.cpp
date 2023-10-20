#include "mesh.hpp"
#include <format>
#include <queue>

#include <nlohmann/json.hpp>
#include <fstream>

namespace assimp_model
{
    constexpr int animation_texture_width = 1024;

    auto Mesh::append_mesh(std::vector<Vertex> &append_vertices, std::vector<unsigned int> &append_indices, std::vector<glm::vec2> &append_driven_bone_offset, std::vector<std::vector<driven_bone>> &append_driven_bone_and_weight) -> void
    {
        auto indices_offset = vertices.size();
        float driven_bone_offset_offset{0.0};
        if (!vertices.empty())
            driven_bone_offset_offset = bone_id_and_weight.size();
        int i{0};
        for (auto &v : append_vertices)
        {
            append_driven_bone_offset[i].x += driven_bone_offset_offset;
            v.bone_weight_offset = append_driven_bone_offset[i];
            i++;
            vertices.emplace_back(v);
        }

        for (auto &b_and_w : append_driven_bone_and_weight)
        {
            for (auto &bw : b_and_w)
            {
                bone_id_and_weight.emplace_back(float(bw.driven_bone_id), bw.driven_bone_weight);
            }
        }
        for (auto i : append_indices)
        {
            indices.emplace_back(indices_offset + i);
        }
    }

    auto Mesh::setup_mesh(bool import_animation) -> void
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
        glBindVertexArray(0);

        if (import_animation) {
            glGenTextures(1, &bone_weight_texture);

            glBindTexture(GL_TEXTURE_2D, bone_weight_texture);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, animation_texture_width, bone_id_and_weight.size() / 2 / animation_texture_width + 1, 0, GL_RGBA, GL_FLOAT, bone_id_and_weight.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindTexture(GL_TEXTURE_2D, 0);
        }


        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, bone_weight_texture);
    }

    auto Model::load_with_config(std::string const path) -> bool
    {
        std::ifstream config_fs(ROOT_DIR + path);
        std::cout << "load config " << path << std::endl;
        auto config = nlohmann::json::parse(config_fs, nullptr, true, true);

        import_animation = config.find("import_animation").value();

        model_path = config.find("model_path").value();

        scale = config.find("scale").value();

        if (import_animation) {
            skeleton_root = config.find("skeleton_root").value();

            play_anim_track = config.find("play_anim_track").value();

            speed = config.find("speed").value();
        }

        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ROOT_DIR + model_path, aiProcess_FlipUVs | aiProcess_SplitByBoneCount);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return false;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        auto processSkeleton = [&]() -> void
        {

            auto bone_root = scene->mRootNode;

            auto walk_bone_tree = [&]() -> void
            {
                std::queue<aiNode *> bone_to_be_walk({bone_root});
                auto find_skeleton_root{false};
                auto is_skeleton_root{false};
                while (!bone_to_be_walk.empty())
                {
                    auto bone_node = bone_to_be_walk.front();
                    bone_to_be_walk.pop();

                    std::string bone_name = bone_node->mName.C_Str();
                    std::string parent_name{};
                    int parent_id{-1};
                    std::vector<int> child_id{};

                    if (bone_name == skeleton_root) {
                        find_skeleton_root = true;
                        is_skeleton_root = true;
                        while (!bone_to_be_walk.empty())
                            bone_to_be_walk.pop();
                    }

                    if (find_skeleton_root) {
                        bone_name_to_id.emplace(bone_name, bone_name_to_id.size());

                        parent_name = bone_node->mParent->mName.C_Str();
                        bones.emplace_back();
                        if (! is_skeleton_root) {

                            parent_id = bone_name_to_id.at(parent_name);
                        }

                        is_skeleton_root = false;
                    }

                    for (auto bone_child_i = 0; bone_child_i < bone_node->mNumChildren; bone_child_i++)
                    {
                        bone_to_be_walk.push(bone_node->mChildren[bone_child_i]);

                        std::string child_name = bone_node->mChildren[bone_child_i]->mName.C_Str();
                        if (find_skeleton_root) {
                            bone_name_to_id.emplace(child_name, bone_name_to_id.size());
                            // bones.emplace_back();
                            child_id.emplace_back(bone_name_to_id.at(child_name));
                        }
                    }
                    if (find_skeleton_root) {
                        bones[bone_name_to_id.at(bone_name)] = {{}, parent_id, bone_name, child_id};
                    }
                }
            };

            walk_bone_tree();

            if (scene->HasAnimations())
            {
                auto anim_num = scene->mNumAnimations;

                tracks.resize(anim_num);

                std::cout << std::format("anim count {:d}\n", anim_num);

                for (auto anim_id = 0; anim_id < anim_num; anim_id++)
                {

                    auto &track = tracks[anim_id];

                    auto anim = scene->mAnimations[anim_id];
                    auto anim_channel_num = anim->mNumChannels;
                    // assert(anim != nullptr);
                    // aiAnimation anim_ins = *scene->mAnimations[anim_id];
                    std::cout << std::format("anim track name {:s}\n", anim->mName.C_Str());

                    track.track_name = std::string(anim->mName.C_Str());
                    track.track_name = track.track_name.substr(track.track_name.find_last_of('|') + 1);
                    track.duration = anim->mDuration;
                    track.frame_per_second = anim->mTicksPerSecond;
                    track.channels.resize(bone_name_to_id.size());
                    assert(anim->mChannels[0]->mNumRotationKeys == anim->mChannels[0]->mNumPositionKeys);
                    assert(anim->mChannels[0]->mNumRotationKeys == anim->mChannels[0]->mNumScalingKeys);

                    std::cout << std::format("anim frames {:d}\n", anim->mChannels[0]->mNumRotationKeys);
                    for (auto i = 0; i < anim_channel_num; i++)
                    {
                        auto& channel_node = anim->mChannels[i];
                        if(bone_name_to_id.find(channel_node->mNodeName.C_Str()) == bone_name_to_id.end()) {
                            std::cout << std::format("channel {:s} not in mapping\n", channel_node->mNodeName.C_Str());
                            abort();
                        }
                        auto& channel_id = bone_name_to_id.at(channel_node->mNodeName.C_Str());
                        auto& channel = track.channels[channel_id];

                        channel.rotations.resize(channel_node->mNumRotationKeys, glm::identity<glm::quat>());
                        channel.positions.resize(channel_node->mNumRotationKeys, glm::vec3(0,0,0));
                        channel.scales.resize(channel_node->mNumRotationKeys, glm::vec3(1.0f, 1.0f, 1.0f));
                        // channel.times.resize(channel_node->mNumRotationKeys);

                        for (auto key_id = 0; key_id < channel_node->mNumRotationKeys; key_id++)
                        {
                            auto& rot = channel_node->mRotationKeys[key_id].mValue;
                            auto& trans = channel_node->mPositionKeys[key_id].mValue;
                            auto& scale = channel_node->mScalingKeys[key_id].mValue;
                            // auto rot_mat = aiMatrix4x4(rot.GetMatrix());

                            channel.positions[key_id] = glm::vec3(trans.x, trans.y, trans.z);
                            channel.rotations[key_id] = glm::quat(rot.w, rot.x, rot.y, rot.z);
                            channel.scales[key_id] = glm::vec3(scale.x, scale.y, scale.z);

                            // channel.trans_matrix[key_id] =
                            //     glm::translate(glm::mat4x4(1.0f), glm::vec3(trans.x, trans.y, trans.z))
                            //     * glm::scale(glm::mat4x4(1.0f), glm::vec3(scale.x, scale.y, scale.z))
                            //     * glm::toMat4(glm::quat(rot.w, rot.x, rot.y, rot.z)) ;

                            // channel.times[key_id] = channel_node->mRotationKeys[key_id].mTime;
                        }
                    }
                }
            }
        };

        if (import_animation)
            processSkeleton();

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
        // uniform_mesh.setup_mesh();
        setup_model();
        return true;
    }

    auto Model::processNode(aiNode *node, const aiScene *scene) -> void
    {
        // process each mesh located at the current node
        // std::cout << std::format("mesh num : {:d}\n", node->mNumMeshes);
        for (unsigned int i = 0; i < scene->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene.
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            // std::cout << (scene->mNumMeshes);
            aiMesh *mesh = scene->mMeshes[i];
            processMesh(mesh, scene);
            // return;
        }
        // return;
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        // for (unsigned int i = 0; i < node->mNumChildren; i++)
        // {
        //     processNode(node->mChildren[i], scene);
        //     // break;
        // }
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

            vertex_idx++;

            vertices.emplace_back(vertex);
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

        if (import_animation) {
            auto &mesh_bones = mesh->mBones;
            auto bone_num = mesh->mNumBones;

            for (auto i = 0; i < bone_num; i++)
            {
                auto &bone = mesh_bones[i];
                auto bone_id = bone_name_to_id.at(bone->mName.C_Str());
                auto bone_drive_vert_num = bone->mNumWeights;

                auto &bone_bind_pose = bone->mOffsetMatrix;
                auto bone_bind_pose_mat = glm::mat4x4{
                    bone_bind_pose.a1, bone_bind_pose.a2, bone_bind_pose.a3, bone_bind_pose.a4,
                    bone_bind_pose.b1, bone_bind_pose.b2, bone_bind_pose.b3, bone_bind_pose.b4,
                    bone_bind_pose.c1, bone_bind_pose.c2, bone_bind_pose.c3, bone_bind_pose.c4,
                    bone_bind_pose.d1, bone_bind_pose.d2, bone_bind_pose.d3, bone_bind_pose.d4,
                };
                if (bones[bone_id].bind_pose_offset_mat == glm::mat4x4{}) {
                    bones[bone_id].bind_pose_offset_mat = bone_bind_pose_mat;
                }
                else if (bones[bone_id].bind_pose_offset_mat != bone_bind_pose_mat)
                {
                    // bones[bone_id].bind_pose_local = bone_bind_pose_mat;
                    std::cout << "error: bind pose conflict\n";
                }

                for (auto j = 0; j < bone_drive_vert_num; j++)
                {
                    auto vert_id = bone->mWeights[j].mVertexId;
                    auto vert_weight = bone->mWeights[j].mWeight;
                    auto &b_and_w = driven_bone_and_weight[vert_id];

                    b_and_w.emplace_back(driven_bone{float(bone_id), vert_weight});
                }
            }
        }

        auto i{0};
        auto base_offset{0};
        for (auto &b_and_w : driven_bone_and_weight)
        {
            driven_bone_offset[i] = glm::vec2{base_offset, b_and_w.size()};
            base_offset += b_and_w.size();
            i++;
        }
        uniform_mesh.append_mesh(vertices, indices, driven_bone_offset, driven_bone_and_weight);
    }

    auto Model::create_bind_pose_matrix_texure() -> void
    {
        std::vector<glm::mat4x4> tmp_bind_mat_array{};
        // tmp_bind_mat_array.resize(bones.size());
        for (auto &bone : bones)
        {
            // std::cout << bone.bind_pose_world[0][0] << std::endl;
            tmp_bind_mat_array.emplace_back(bone.bind_pose_offset_mat);

        }

        glGenTextures(1, &bind_pose_texture);
        glBindTexture(GL_TEXTURE_2D, bind_pose_texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, animation_texture_width, tmp_bind_mat_array.size() * 4 / animation_texture_width + 1, 0, GL_RGBA, GL_FLOAT, tmp_bind_mat_array.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    auto Model::create_anim_matrix_texure(std::vector<int>& frame_ids, std::vector<int>& track_id, float left_weight, float right_weight, std::vector<float>& weights) -> void
    {
        // assert(track_index < tracks.size());
        auto bone_num = bone_name_to_id.size();
        // auto &track_anim_texture = tracks[track_index].track_anim_texture;
        auto current_frame = std::vector<Bone_Trans>{};
        current_frame.resize(bone_num);

        for (int i = 0; i < bone_num; i++) {
            auto trans = glm::vec3{};
            auto rotation = std::vector<glm::quat>{};
            auto scale = glm::vec3{};

            for (int j = 0; j < weights.size(); j++) {
                auto& channel = tracks[track_id[j]].channels[i];
                auto& frame_id = frame_ids[track_id[j]];
                auto& trans_l = channel.positions[frame_id    ];
                auto& trans_r = channel.positions[frame_id + 1];

                auto& rotation_l = channel.rotations[frame_id    ];
                auto& rotation_r = channel.rotations[frame_id + 1];

                auto& scale_l = channel.scales[frame_id    ];
                auto& scale_r = channel.scales[frame_id + 1];

                trans += weights[j] * (left_weight * trans_l + right_weight * trans_r);
                rotation.emplace_back((glm::slerp(rotation_l, rotation_r, right_weight)));
                scale += weights[j] * (left_weight * scale_l + right_weight * scale_r);
            }

            current_frame[i].position = trans;
            auto tmp_quat = weights[0] + weights[1] > 0.0f ? glm::slerp(rotation[0], rotation[1], weights[1] / (weights[0] + weights[1])) : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            current_frame[i].rotation = glm::slerp(tmp_quat, rotation[2], weights[2] / (weights[0] + weights[1] + weights[2]));
            current_frame[i].scale = scale;
        }

        // auto channel_num = channels.size();

        std::vector<glm::mat4x4> tmp_anim_pose_frames{};

        tmp_anim_pose_frames.resize(bone_num, glm::identity<glm::mat4x4>());

        auto get_world_transform = [&](int bone_id) -> glm::mat4x4 {
            auto bone_it{bone_id};
            glm::quat world_rotation = glm::identity<glm::quat>();
            glm::vec3 world_transform = glm::vec3();
            glm::vec3 world_scale = glm::vec3(1.0f, 1.0f, 1.0f);

            while (bone_it != -1) {
                auto& current_bone = current_frame[bone_it];
                auto tmp_world_transform = current_bone.position + current_bone.rotation * (current_bone.scale * world_transform);
                auto tmp_world_scale = current_bone.scale * world_scale;
                auto tmp_world_rotation = current_bone.rotation * world_rotation;

                world_transform = tmp_world_transform;
                world_rotation = tmp_world_rotation;
                world_scale = tmp_world_scale;

                bone_it = bones[bone_it].parent_id;
            }
            return glm::translate(glm::mat4x4(1.0f), world_transform)
                    * glm::scale(glm::mat4x4(1.0f), world_scale)
                    * glm::toMat4(world_rotation);
        };

        for (auto i = 0; i < bone_num; i++)
        {
            tmp_anim_pose_frames[i] = get_world_transform(i);
        }
        if (track_anim_texture == 0)
            glGenTextures(1, &track_anim_texture);

        // glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, track_anim_texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tmp_anim_pose_frames.size() * 4, 1, 0, GL_RGBA, GL_FLOAT, tmp_anim_pose_frames.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);

    }

    auto Model::bind_textures() -> void
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, uniform_mesh.bone_weight_texture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bind_pose_texture);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, track_anim_texture);


        // auto track_index{0};
        // for (auto &track : tracks)
        // {
        //     glActiveTexture(GL_TEXTURE2 + track_index);
        //     glBindTexture(GL_TEXTURE_2D, track.track_anim_texture);
        //     track_index++;
        // }
    }
} // namespace model
