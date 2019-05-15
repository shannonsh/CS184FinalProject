#version 330

uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

uniform vec4 u_color;

uniform sampler2D u_texture_2;
uniform vec2 u_texture_2_size;

uniform float u_normal_scaling;
uniform float u_height_scaling;

in vec4 v_position;
in vec4 v_normal;
in vec4 v_tangent;
in vec2 v_uv;

out vec4 out_color;

float h(vec2 uv) {
  // You may want to use this helper function...
  return texture(u_texture_2, uv).r;
}

void main() {
  // YOUR CODE HERE
  // (Placeholder code. You will want to replace it.)
  float w_ = u_texture_2_size[0];
  float h_ = u_texture_2_size[1];
  float u = v_uv[0];
  float v = v_uv[1];
  float dU = (h(vec2(u+1/w_, v)) - h(v_uv)) * u_height_scaling * u_normal_scaling;
  float dV = (h(vec2(u, v+1/h_)) - h(v_uv)) * u_height_scaling * u_normal_scaling;
  vec3 n0 = vec3(-dU, -dV, 1);
  vec3 b = cross(v_normal.xyz, v_tangent.xyz).xyz;
  mat3 TBN = mat3(v_tangent.x, b.x, v_normal.x,
  			      v_tangent.y, b.y, v_normal.y,
  			      v_tangent.z, b.z, v_normal.z);
  vec3 nb = n0 * TBN;

  // Do phong shading
  vec3 h = normalize((normalize(u_light_pos - v_position.xyz) + normalize(u_cam_pos - v_position.xyz))/2);
  vec3 l = u_light_pos - v_position.xyz;
  float r = length(l);
  l = normalize(l);
  vec3 IR2 = (u_light_intensity / (r*r));
  out_color.xyz = vec3(.1, .1, .1) + IR2 * max(0, dot(l, nb)) 
  					+ IR2 * pow(max(0, dot(nb, h)), 100);
  out_color.a = 1;
}
