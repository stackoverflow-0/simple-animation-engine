#version 430

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
// uniform mat3 normalMatrix;
uniform float time;

// uniform sampler2D bone_and_weight;
// uniform sampler2D bone_trans_matrix;

uniform sampler2D bone_id_and_weight;

uniform sampler2D bone_bind_pose;

uniform sampler2D bone_current_pose;

void main()
{
    int base_idx = int(bone_weight_offset.x);
    int bone_num = int(bone_weight_offset.y);
    mat4 bind_mat = mat4(0);
    mat4 current_mat = mat4(0);
    weight = 0;
    for (int i = 0; i < bone_num; i++) {
        vec4 bw = texelFetch(bone_id_and_weight, ivec2((base_idx + i) / 2 % 1024, (base_idx + i) / 2 / 1024), 0);
        float bone_weight = (base_idx + i) % 2 == 0 ? bw.y : bw.w;
        int bone_id = (base_idx + i) % 2 == 0 ? int(bw.x) : int(bw.z);
        int bone_offset = bone_id * 4;
        vec4 ma = texelFetch(bone_bind_pose, ivec2((bone_offset    ) % 1024, (bone_offset    ) / 1024), 0);
        vec4 mb = texelFetch(bone_bind_pose, ivec2((bone_offset + 1) % 1024, (bone_offset + 1) / 1024), 0);
        vec4 mc = texelFetch(bone_bind_pose, ivec2((bone_offset + 2) % 1024, (bone_offset + 2) / 1024), 0);
        vec4 md = texelFetch(bone_bind_pose, ivec2((bone_offset + 3) % 1024, (bone_offset + 3) / 1024), 0);

        bind_mat += bone_weight * transpose(mat4(ma, mb, mc, md));

        vec4 ca = texelFetch(bone_current_pose, ivec2((bone_offset    ), 1), 0);
        vec4 cb = texelFetch(bone_current_pose, ivec2((bone_offset + 1), 1), 0);
        vec4 cc = texelFetch(bone_current_pose, ivec2((bone_offset + 2), 1), 0);
        vec4 cd = texelFetch(bone_current_pose, ivec2((bone_offset + 3), 1), 0);

        current_mat += bone_weight * transpose(mat4(ca, cb, cc, cd));

        weight += abs(bone_weight) ;
    }
    mat4 bone_trans_mat = mat4(1.0);
    bone_trans_mat =  bind_mat;
    bone_trans_mat = current_mat * bind_mat;

    // weight = 0.5 *weight;
    if (weight > 1.001)
        weight = -1.0;
    if (weight < 0.999)
        weight = -1.0;

    // weight = bone_weight_offset.x + bone_weight_offset.y;
    o_position = vec3(world * vec4(position, 1.0));
    o_normal   = (inverse(transpose(world * bone_trans_mat)) * vec4(normal, 1.0)).xyz;
    o_texcoord = texcoord.xy;

    gl_Position = viewProj * world * bone_trans_mat * vec4(position, 1.0);
}
