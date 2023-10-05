#version 430

in vec3 o_position;
in vec3 o_normal;
in vec2 o_texcoord;

layout(location = 0) out vec4 fragColor;

uniform vec4 gizmo_color;

void main()
{
    // vec3 n = normalize(o_normal);
	// vec3 frag_pos = normalize(cam_pos - o_position);
	
	// vec3 finalColor = texture(tex_sampler, o_texcoord).xyz;
	// vec3 finalColor = vec3(0.5, 0.5, 0.5);
	// finalColor *= max(0.0, dot(n, frag_pos));
	
	fragColor = gizmo_color;
}