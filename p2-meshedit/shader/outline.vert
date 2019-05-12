

void main()
{
  float thickness = 0.015;
  gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * (gl_Vertex + thickness * vec4(gl_Normal, 0));
    
}
