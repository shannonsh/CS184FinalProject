uniform int outputID;
uniform vec3 eyePos;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform vec3 ambientColor;
uniform float n;

uniform sampler2D u_texture_1;
//vec2 in_uv;


varying vec3 normal;
varying vec3 vertex;

varying vec2 uv;

#define PI 3.1415926

vec3 shadeDiffuseFromEye();

vec3 shadePhong(vec3 lightPos);

vec3 shadeEnvmapReflection();


void main()
{
    if(outputID == 0)
    {
        gl_FragColor = vec4(shadeDiffuseFromEye(), 1.0);
        return;
    }
    if(outputID == 1)
    {
        gl_FragColor = vec4(shadePhong(vec3(10, -10, 10)), 1.0);
        return;
    }
    if(outputID == 2)
    {
        // Do something interesting here for extra credit.
        // gl_FragColor = vec4(???, 1.0);
        // return;

        vec3 v_position3 = vertex.xyz;
  
          float intensity;
          vec3 color;
          vec3 lightDir = vec3(10.0,10.0,10.0) - v_position3;
          intensity = dot(lightDir,normalize(normal));
          if (intensity > 0.95) color = vec3(1.0,0.5,0.5);
          else if (intensity > 0.5) color = vec3(0.6,0.3,0.3);
          else if (intensity > 0.25) color = vec3(0.4,0.2,0.2);
          else color = vec3(0.2,0.1,0.1);
          vec3 view_dir = eyePos - vertex.xyz;


        gl_FragColor.xyz = color;
        gl_FragColor.a = 1.0;
	    return;
    }
    if(outputID == 3)
    {
    vec3 v_position3 = vertex.xyz;
  
        float intensity;
        vec3 lightDir = vec3(10.0,10.0,10.0) - v_position3;
        intensity = dot(normalize(lightDir),normalize(normal));
        gl_FragColor.xyz = diffuseColor;

        float maxval = max(gl_FragColor.x, gl_FragColor.y);
        maxval = max(maxval, gl_FragColor.z);
        if (maxval == 0.0) maxval = -1.0; 
        vec3 shadecolor = vec3(1,1,0); //COLOR OF THE SHADOW




        for (float i = 1.0/n; i < 1.0; i += 1.0/n) {
            if (intensity < i) gl_FragColor.xyz -= vec3(maxval/n,maxval/n,maxval/n) * shadecolor;
        }

        
        vec3 view_dir = eyePos - vertex.xyz;
        //gl_FragColor.xyz = diffuseColor + addon;
        gl_FragColor.a = 1.0;
        return;
    }
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

    // "Constants" to play with for coloring and lighting
    float p = 10.0; // Used for specular shading
    // vec3 lightColor = vec3(20.0 / 255.0, 200.0 / 255.0, 250.0 / 255.0);
//    vec3 lightColor = vec3(175.0 / 251.0, 200.0 / 255.0, 79.0 / 255.0);
  vec3 lightColor = texture2D(u_texture_1, gl_TexCoord[0].st).xyz;

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

    // Diffuse component
//    float diffuseDot = dot(n, lightDir);

    // cel shading
    float intensity = dot(lightDir,normalize(normal));
    if (intensity > 0.4) intensity = 1.0;
    else if (intensity > 0.3) intensity = 0.5;
    else if (intensity > 0.2) intensity = 0.2;
    else intensity = 0.1;

    // add outline around object if camera perp to normal
    vec3 view_dir = eyePos - vertex.xyz;
//    if(abs(dot(view_dir, normal)) < 0.4) {
//      intensity = 0.0;
//    }
    vec3 diffuse = lightColor * clamp(intensity, 0.0, 1.0);

    // Specular component
    vec3 halfAngle = normalize(outDir + lightDir);
    vec3 specularColor = min(lightColor + 0.5, 1.0);
    float specularDot = dot(n, halfAngle);
    // cel shading
    if (specularDot > 0.99) specularDot = 1.0;
    else if (specularDot > 0.5) specularDot = 0.3;
    else if (specularDot > 0.25) specularDot = 0.05;
    else specularDot = 0.01;
    vec3 specular = specularColor * pow(clamp(specularDot, 0.0, 1.0), p);
    // specularColor is "k_s"

    // Phong shading is the summation of diffuse, ambient, and specular shading components.
    return diffuse + ambient + specular;
}
