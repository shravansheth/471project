#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

out vec3 vPos;
out vec3 vNormal;
out vec2 vTex;

void main()
{
    vec4 worldPos = M * vec4(position, 1.0);
    vPos = worldPos.xyz;

    vNormal = mat3(transpose(inverse(M))) * normal;

    vTex = texcoord;

    gl_Position = P * V * worldPos;
}