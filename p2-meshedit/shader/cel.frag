uniform int outputID;
uniform vec3 eyePos;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform vec3 ambientColor;
uniform float n;
uniform bool tex_bool;
uniform vec3 light_position;

uniform sampler2D u_texture_1;
//vec2 in_uv;


varying vec3 normal;
varying vec3 vertex;

varying vec2 uv;

#define PI 3.1415926

vec3 shadeDiffuseFromEye();

vec3 shadePhong(vec3 lightPos);

vec3 shadeEnvmapReflection();

void multiLevelShade(vec3 shadecolor);


void main()
{
    if(outputID == 0)
    {
        gl_FragColor = vec4(shadeDiffuseFromEye(), 1.0);
        return;
    }
    if(outputID == 1)
    {
        gl_FragColor = vec4(shadePhong(eyePos), 1.0);
        return;
    }
    if(outputID == 2)
    {
    multiLevelShade(vec3(0.4,1,1)); //red
    }
    if(outputID == 3)
    {
    multiLevelShade(vec3(0.4,0.4,1)); //yellow
    }
    if(outputID == 4)
    {
    multiLevelShade(vec3(1,0.4,1)); //green
    }
    if(outputID == 5)
    {
    multiLevelShade(vec3(1,0.4,0.4)); //cyan
    }
    if(outputID == 6)
    {
    multiLevelShade(vec3(1,1,0)); //blue
    }
    if(outputID == 7)
    {
    multiLevelShade(vec3(0.4,1,0.4)); //purple
    }
    if(outputID == 8)
    {
    multiLevelShade(vec3(0.5,0.5,0.5));
    }
    if(outputID == 9)
    {
    multiLevelShade(vec3(1,1,1));
    }
}

void multiLevelShade(vec3 shadecolor) {
    vec3 v_position3 = vertex.xyz;
  
        float intensity;
        vec3 lightDir = eyePos - v_position3;
        intensity = dot(normalize(lightDir),normalize(normal));
        if (tex_bool) {
            gl_FragColor.xyz = texture2D(u_texture_1, gl_TexCoord[0].st).xyz;
        } else {
            gl_FragColor.xyz = diffuseColor; 
        }

        float maxval = max(gl_FragColor.x, gl_FragColor.y);
        maxval = max(maxval, gl_FragColor.z);
        if (maxval == 0.0) maxval = -1.0; 
        //vec3 shadecolor = vec3(1,1,0); //COLOR OF THE SHADOW

        for (float i = 1.0/n; i < 1.0; i += 1.0/n) {
            if (intensity < i) gl_FragColor.xyz -= vec3(maxval/n,maxval/n,maxval/n) * shadecolor;
            gl_FragColor.x = clamp(gl_FragColor.x, 0.0, 1.0);
            gl_FragColor.y = clamp(gl_FragColor.y, 0.0, 1.0);
            gl_FragColor.z = clamp(gl_FragColor.z, 0.0, 1.0);
        }

        
        vec3 view_dir = eyePos - vertex.xyz;
        //gl_FragColor.xyz = diffuseColor + addon;
        gl_FragColor.a = 1.0;
        return;
}

vec3 shadeDiffuseFromEye()
{
    vec3 n = normalize(normal);
    vec3 out_vec = normalize(eyePos - vertex);
    float diffuse = abs(dot(out_vec, n));
    return vec3(diffuse);
}

vec3 shadePhong(vec3 lightPos)
{
    // TODO Part 7.
    // TODO Compute Phong shading here. You can choose any color you like. But make
    // TODO sure that you have noticeable specular highlights in the shading.
    // TODO Variables to use: eyePos, lightPos, normal, vertex


    ////////////////////////////////////////
    // TODO: REMOVE BEFORE RELEASING.
    ////////////////////////////////////////
  lightPos.z += 10.0;
    // "Constants" to play with for coloring and lighting
    float p = 5.0; // Used for specular shading
//    vec3 lightColor = vec3(175.0 / 251.0, 200.0 / 255.0, 79.0 / 255.0);
  vec3 lightColor = texture2D(u_texture_1, gl_TexCoord[0].st).xyz;
//  vec3 lightColor = vec3(gl_TexCoord[0].s, gl_TexCoord[0].t, 0.0);
//  vec3 lightColor = diffuseColor;

    if (tex_bool) {
            lightColor = texture2D(u_texture_1, gl_TexCoord[0].st).xyz;
      lightColor = vec3(0.104, 0.333, 0.560);
        } else {
            lightColor = diffuseColor; 
        }

    // Useful vectors for shading, some normalized.
    vec3 lightVec = lightPos - vertex;
    vec3 lightDir = normalize(lightVec); // "l" in slides
    vec3 outDir = normalize(eyePos - vertex); // "v" in slides
    vec3 n = normalize(normal); // "n" in slides

    // Compute the "1/r^2" term to simulate light intensity falling off as the light source moves further away
    // distFactor is not used in this Phong shading since it makes the environment unusually dark; therefore, the factor is omitted entirely in the following equations
    float distFactor = 1.0 / sqrt(dot(lightVec, lightVec));

    // Ambient component
  vec3 ambient = ambientColor;

    // Diffuse component (now cel shading)
//    float diffuseDot = dot(n, lightDir);

    // cel shading
    float intensity = dot(lightDir,normalize(normal));
    if (intensity > 0.7) intensity = 1.0;
    else if (intensity > 0.5) intensity = 0.7;
    else if (intensity > 0.3) intensity = 0.3;
    else if (intensity > 0.2) intensity = 0.2;
    else intensity = 0.1;
//    intensity = 1.0; // for debugging
    vec3 diffuse = lightColor * clamp(intensity, 0.0, 1.0);

    // Specular component
    vec3 halfAngle = normalize(outDir + lightDir);
    vec3 specularColor = min(lightColor + 0.5, 1.0);
    float specularDot = dot(n, halfAngle);
    // cel shading
    if (specularDot > 0.95) specularDot = 1.0;
    else if (specularDot > 0.5) specularDot = 0.3;
    else if (specularDot > 0.25) specularDot = 0.05;
    else specularDot = 0.01;
    vec3 specular = specularColor * pow(clamp(specularDot, 0.0, 1.0), p);
    // specularColor is "k_s"
  // TODO: decide whether to prioritize textures > color or specify manually to use texture or color

    // Phong shading is the summation of diffuse, ambient, and specular shading components.
    return diffuse + ambient + specular;
}
