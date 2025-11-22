#version 330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

out vec2 vTexCoord;
out vec3 fragNor;
out vec3 EPos;

void main()
{
    vec4 posW = M * vertPos;
    vec4 posV = V * posW;

    gl_Position = P * posV;
    vTexCoord = vertTex;

    mat3 N = transpose(inverse(mat3(V * M)));
    fragNor = normalize(N * vertNor);
    EPos = posV.xyz;
}