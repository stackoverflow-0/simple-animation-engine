#version 430

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec4 driven_bone_id;
layout(location = 4) in vec4 driven_bone_weight;
// layout (binding = 0) uniform bone_buffer {
//     vec4 b_and_w[];
// };

out vec3 o_position;
out vec3 o_normal;
out vec2 o_texcoord;

out float weight;
	
uniform mat4 world;
uniform mat4 viewProj;
uniform mat3 normalMatrix;
uniform float time;

uniform sampler2D bone_and_weight;
uniform sampler2D bone_trans_matrix;

void main()
{
	o_position = vec3(world * vec4(position, 1.0));
    o_normal   = normalMatrix * normal;
    o_texcoord = texcoord.xy;

    weight = driven_bone_weight.x + driven_bone_weight.y + driven_bone_weight.z + driven_bone_weight.w;
	
    gl_Position = viewProj * world * vec4(position, 1.0);
}