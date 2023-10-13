#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec2 bone_weight_offset;

out vec3 o_position;
out vec3 o_normal;
out vec2 o_texcoord;
out vec4 o_color;

uniform mat4 world;
uniform mat4 viewProj;

// uniform sampler2D bone_bind_pose;

uniform sampler2D bone_current_poses[3];
uniform int frame_ids[3];

uniform int blend_anim_num;

uniform float left_weight;
uniform float right_weight;

uniform float blend_weights[3];

uniform int bone_id;

uniform float gizmo_scale;
uniform vec4 gizmo_color;
uniform int show_bone_weight_id;

void main()
{
    int bone_offset = bone_id * 4;

    // vec4 ma = texelFetch(bone_bind_pose, ivec2((bone_offset    ) % 1024, (bone_offset    ) / 1024), 0);
    // vec4 mb = texelFetch(bone_bind_pose, ivec2((bone_offset + 1) % 1024, (bone_offset + 1) / 1024), 0);
    // vec4 mc = texelFetch(bone_bind_pose, ivec2((bone_offset + 2) % 1024, (bone_offset + 2) / 1024), 0);
    // vec4 md = texelFetch(bone_bind_pose, ivec2((bone_offset + 3) % 1024, (bone_offset + 3) / 1024), 0);
    //  left frame
//     vec4 ca_l = texelFetch(bone_current_pose, ivec2((bone_offset    ), frame_id), 0);
//     vec4 cb_l = texelFetch(bone_current_pose, ivec2((bone_offset + 1), frame_id), 0);
//     vec4 cc_l = texelFetch(bone_current_pose, ivec2((bone_offset + 2), frame_id), 0);
//     vec4 cd_l = texelFetch(bone_current_pose, ivec2((bone_offset + 3), frame_id), 0);
//     // right frame
//     vec4 ca_r = texelFetch(bone_current_pose, ivec2((bone_offset    ), frame_id + 1), 0);
//     vec4 cb_r = texelFetch(bone_current_pose, ivec2((bone_offset + 1), frame_id + 1), 0);
//     vec4 cc_r = texelFetch(bone_current_pose, ivec2((bone_offset + 2), frame_id + 1), 0);
//     vec4 cd_r = texelFetch(bone_current_pose, ivec2((bone_offset + 3), frame_id + 1), 0);
//
    mat4 bone_trans_mat = mat4(0.0);
//     bone_trans_mat = (left_weight * mat4(ca_l, cb_l, cc_l, cd_l) + right_weight * mat4(ca_r, cb_r, cc_r, cd_r));
    float blend_weights_sum = 0.0;
    for (int k = 0; k < blend_anim_num; k++) {
        mat4 current_mat = mat4(0);
        int bone_offset = bone_id * 4;
        // left frame
        vec4 ca_l = texelFetch(bone_current_poses[k], ivec2((bone_offset    ), frame_ids[k]), 0);
        vec4 cb_l = texelFetch(bone_current_poses[k], ivec2((bone_offset + 1), frame_ids[k]), 0);
        vec4 cc_l = texelFetch(bone_current_poses[k], ivec2((bone_offset + 2), frame_ids[k]), 0);
        vec4 cd_l = texelFetch(bone_current_poses[k], ivec2((bone_offset + 3), frame_ids[k]), 0);
        // right frame
        vec4 ca_r = texelFetch(bone_current_poses[k], ivec2((bone_offset    ), frame_ids[k] + 1), 0);
        vec4 cb_r = texelFetch(bone_current_poses[k], ivec2((bone_offset + 1), frame_ids[k] + 1), 0);
        vec4 cc_r = texelFetch(bone_current_poses[k], ivec2((bone_offset + 2), frame_ids[k] + 1), 0);
        vec4 cd_r = texelFetch(bone_current_poses[k], ivec2((bone_offset + 3), frame_ids[k] + 1), 0);

        current_mat += (left_weight * mat4(ca_l, cb_l, cc_l, cd_l) + right_weight * mat4(ca_r, cb_r, cc_r, cd_r));
        // bind_mat = bind_mat / total_weight;
        blend_weights_sum += blend_weights[k];
        bone_trans_mat += blend_weights[k] * current_mat;
    }
    bone_trans_mat /= blend_weights_sum;

    o_position = vec3(world * bone_trans_mat * vec4(position, 1.0));
    o_normal   = (inverse(transpose(world * bone_trans_mat)) * vec4(normal, 1.0)).xyz;
    o_texcoord = texcoord.xy;
    o_color = show_bone_weight_id == bone_id ? vec4(1.0, 0.0, 1.0, 1.0) : gizmo_color;

    gl_Position =  viewProj * world * bone_trans_mat * vec4(gizmo_scale * position, 1.0);
}
