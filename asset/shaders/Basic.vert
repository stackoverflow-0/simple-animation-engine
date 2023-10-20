#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec2 bone_weight_offset;

// #ifndef MAX_bone_id_and_weight_LEN
// #error MAX_bone_id_and_weight_LEN undefined
// #endif

// layout (binding = 0) uniform bone_buffer {
//     vec2 bone_id_and_weight[MAX_bone_id_and_weight_LEN];
// };

out vec3 o_position;
out vec3 o_normal;
out vec2 o_texcoord;

out float weight;

uniform mat4 world;
uniform mat4 viewProj;

uniform bool import_animation;

uniform sampler2D bone_id_and_weight;

uniform sampler2D bone_bind_pose;

uniform sampler2D bone_current_pose;

// uniform int blend_anim_num;

// uniform int frame_ids[3];

// uniform float left_weight;
// uniform float right_weight;

// uniform float blend_weights[3];

// uniform bool show_bone_weight;
uniform int show_bone_weight_id;

void main()
{
    // TODO : blend matrix should be done in local space, not in world space. Blending in world space will cause shrink problem.

    mat4 bone_trans_mat = mat4(1.0);
    weight = 0;
    if (import_animation == true) {
        int base_idx = int(bone_weight_offset.x);
        int bone_num = int(bone_weight_offset.y);
        // mat4 bind_mat = mat4(0);
        float blend_weights_sum = 0.0;
        mat4 current_mat = mat4(0);
        // for (int k = 0; k < blend_anim_num; k++) {
            // current_mat = mat4(0);
        float total_weight = 0.0;
        for (int i = 0; i < bone_num; i++) {
            vec4 bw = texelFetch(bone_id_and_weight, ivec2((base_idx + i) / 2 % 1024, (base_idx + i) / 2 / 1024), 0);
            float bone_weight = (base_idx + i) % 2 == 0 ? bw.y : bw.w;
            int bone_id = (base_idx + i) % 2 == 0 ? int(bw.x) : int(bw.z);
            int bone_offset = bone_id * 4;
            vec4 ma = texelFetch(bone_bind_pose, ivec2((bone_offset    ) % 1024, (bone_offset    ) / 1024), 0);
            vec4 mb = texelFetch(bone_bind_pose, ivec2((bone_offset + 1) % 1024, (bone_offset + 1) / 1024), 0);
            vec4 mc = texelFetch(bone_bind_pose, ivec2((bone_offset + 2) % 1024, (bone_offset + 2) / 1024), 0);
            vec4 md = texelFetch(bone_bind_pose, ivec2((bone_offset + 3) % 1024, (bone_offset + 3) / 1024), 0);
            // left frame
            vec4 ca_l = texelFetch(bone_current_pose, ivec2((bone_offset    ), 0), 0);
            vec4 cb_l = texelFetch(bone_current_pose, ivec2((bone_offset + 1), 0), 0);
            vec4 cc_l = texelFetch(bone_current_pose, ivec2((bone_offset + 2), 0), 0);
            vec4 cd_l = texelFetch(bone_current_pose, ivec2((bone_offset + 3), 0), 0);
            // right frame
            // vec4 ca_r = texelFetch(bone_current_pose, ivec2((bone_offset    ), frame_ids[k] + 1), 0);
            // vec4 cb_r = texelFetch(bone_current_pose, ivec2((bone_offset + 1), frame_ids[k] + 1), 0);
            // vec4 cc_r = texelFetch(bone_current_pose, ivec2((bone_offset + 2), frame_ids[k] + 1), 0);
            // vec4 cd_r = texelFetch(bone_current_pose, ivec2((bone_offset + 3), frame_ids[k] + 1), 0);

            // current_mat += bone_weight * (left_weight * mat4(ca_l, cb_l, cc_l, cd_l) + right_weight * mat4(ca_r, cb_r, cc_r, cd_r)) * transpose(mat4(ma, mb, mc, md));
            current_mat += bone_weight * mat4(ca_l, cb_l, cc_l, cd_l) * transpose(mat4(ma, mb, mc, md));

            total_weight += bone_weight;

            if (bone_id == show_bone_weight_id)
                weight += bone_weight;
        }
            // bind_mat = bind_mat / total_weight;
            // blend_weights_sum += blend_weights[k];
            // bone_trans_mat += blend_weights[k] * current_mat / total_weight;
        bone_trans_mat = current_mat / total_weight;
        // bone_trans_mat = mat4(1.0) + 0.0001 * bone_trans_mat;
        // }
        // bone_trans_mat /= blend_weights_sum;
        //  += current_mat;
        // weight /= float(blend_anim_num);
    }

    // if (weight > 1.001)
    //     weight = -1.0;
    // if (weight < 0.999)
    //     weight = -1.0;

    o_position = vec3(world * bone_trans_mat * vec4(position, 1.0));
    o_normal   = (inverse(transpose(world * bone_trans_mat)) * vec4(normal, 1.0)).xyz;
    o_texcoord = texcoord.xy;

    gl_Position = viewProj * world * bone_trans_mat * vec4(position, 1.0);
}
