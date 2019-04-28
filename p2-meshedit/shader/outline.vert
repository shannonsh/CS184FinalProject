

void main()
{
  float thickness = 0.01;
  gl_Position = gl_ModelViewProjectionMatrix * (gl_Vertex + thickness * vec4(gl_Normal, 0));
    
}
