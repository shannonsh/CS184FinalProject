varying vec3 normal;
varying vec3 vertex;
varying vec2 uv;

//out vec2 v_texCoord;

void main()
{
    uv = gl_Vertex.xy;
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    //gl_TexCoord[0] = gl_MultiTexCoord0;
  
    vertex = gl_Vertex.xyz;
    normal = gl_Normal.xyz;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
