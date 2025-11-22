#version  330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightPos;

out vec3 fragNor;
out vec3 lightDir;
out vec3 EPos;

void main()
{
    vec4 posW = M * vertPos;
    vec4 posV = V * posW;
    gl_Position = P * posV;

    mat3 N = transpose(inverse(mat3(V * M)));
    fragNor = normalize(N * vertNor);

    vec3 lightPosV = (V * vec4(lightPos, 1.0)).xyz;
    lightDir = lightPosV - posV.xyz;
    EPos = posV.xyz;
}