#version 330

uniform vec4 u_color;

uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;


uniform sampler2D u_texture_1;
uniform sampler2D u_texture_2;
uniform sampler2D u_texture_3;
uniform sampler2D u_texture_4;
uniform sampler2D u_texture_5;
uniform sampler2D u_texture_6;

in vec4 v_position;
in vec4 v_normal;
in vec2 v_uv;

out vec4 out_color;

float level = 1. / 5.;

float interp(float I, float n) {
	return (1./level) * (I - n * level);
}

void main() {
  // YOUR CODE HERE
  
  // (Placeholder code. You will want to replace it.)
	vec3 tex1 = texture(u_texture_1, v_uv).xyz;
	vec3 tex2 = texture(u_texture_2, v_uv).xyz;
	vec3 tex3 = texture(u_texture_3, v_uv).xyz;
	vec3 tex4 = texture(u_texture_4, v_uv).xyz;
	vec3 tex5 = texture(u_texture_5, v_uv).xyz;
	vec3 tex6 = texture(u_texture_6, v_uv).xyz;

	vec3 h = normalize((normalize(u_light_pos - v_position.xyz) + normalize(u_cam_pos - v_position.xyz))/2);
	vec3 l = u_light_pos - v_position.xyz;
	float r = length(l);
	l = normalize(l);
	vec3 IR2 = (u_light_intensity / (r*r));
	vec3 phong = vec3(.05, .05, .05) + IR2 * max(0, dot(l, v_normal.xyz))
					+ IR2 * pow(max(0, dot(v_normal.xyz, h)), 20);
	phong = clamp(phong, vec3(0.), vec3(1.));
	float I = length(phong);
	// if (I > 0 && I < level)
	// 	 out_color.xyz = mix(tex6, tex5, interp(I, 0));
	// else
	if (I <= 1.*level)
		out_color.xyz = mix(tex5, tex4, interp(I, 0));
	else if (I <= 2.*level)
		out_color.xyz = mix(tex4, tex3, interp(I, 1));
	else if (I <= 3.*level)
		out_color.xyz = mix(tex3, tex2, interp(I, 2));
	else if (I <= 4.*level)
		out_color.xyz = mix(tex2, tex1, interp(I, 3));
	else
		out_color.xyz = mix(tex1, vec3(1.), interp(I, 4));
	out_color.a = 1;
}
