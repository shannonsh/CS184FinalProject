#version 330

uniform vec4 u_color;
uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

in vec4 v_position;
in vec4 v_normal;
in vec2 v_uv;

out vec4 out_color;

void main() {
  // YOUR CODE HERE
  
  // (Placeholder code. You will want to replace it.)
  vec3 h = normalize((normalize(u_light_pos - v_position.xyz) + normalize(u_cam_pos - v_position.xyz))/2);
  vec3 l = u_light_pos - v_position.xyz;
  float r = length(l);
  l = normalize(l);
  vec3 IR2 = (u_light_intensity / (r*r));
  out_color.xyz = vec3(.1, .1, .1) + IR2 * max(0, dot(l, v_normal.xyz))
  					+ IR2 * pow(max(0, dot(v_normal.xyz, h)), 100);
  out_color.a = 1;
}
