#version 330

uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

uniform sampler2D u_texture_1;

in vec4 v_position;
in vec4 v_screen_position;
in vec4 v_normal;
in vec2 v_uv;

out vec4 out_color;

void main() {
  // YOUR CODE HERE
  vec3 v_position3 = v_position.xyz;
  vec3 normal = v_normal.xyz;
  
  float intensity;
  vec4 color;
  vec3 lightDir = u_light_pos - v_position3;
  intensity = dot(lightDir,normalize(normal));
  if (intensity > 0.95) color = vec4(1.0,0.5,0.5,1.0);
  else if (intensity > 0.5) color = vec4(0.6,0.3,0.3,1.0);
  else if (intensity > 0.25) color = vec4(0.4,0.2,0.2,1.0);
  else color = vec4(0.2,0.1,0.1,1.0);
  vec3 view_dir = u_cam_pos - v_position.xyz;
  if(abs(dot(view_dir, v_normal.xyz)) < 0.3) color = vec4(0, 0, 0, 1);
  out_color = color;
}
