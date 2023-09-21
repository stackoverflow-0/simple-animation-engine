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
uniform mat3 normalMatrix;
uniform float time;

// uniform sampler2D bone_and_weight;
// uniform sampler2D bone_trans_matrix;

uniform sampler2D bone_id_and_weight;

void main()
{
	o_position = vec3(world * vec4(position, 1.0));
    o_normal   = normalMatrix * normal;
    o_texcoord = texcoord.xy;

    int base_idx = int(bone_weight_offset.x);
    int bone_num = int(bone_weight_offset.y);
    weight = 0; 
    for (int i = 0; i < bone_num; i++) {
        vec4 bw = texelFetch(bone_id_and_weight, ivec2((base_idx + i) / 2 % 1024, (base_idx + i) / 2 / 1024), 0);
        float w = (base_idx + i) % 2 == 0 ? bw.y : bw.w;
        weight += w;
    }
    // weight = 1.0;
    if (weight > 1.001)
        weight = -1.0;
    if (weight < 0.999)
        weight = -1.0;

    // weight = bone_weight_offset.x + bone_weight_offset.y;
	
    gl_Position = viewProj * world * vec4(position, 1.0);
}