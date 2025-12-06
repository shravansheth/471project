#version 330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

out vec2 vTexCoord;
out vec3 fragNor;
out vec3 EPos;     // position in eye space

void main()
{
    // Compute world space position
    vec4 posW = M * vertPos;

    // Transform to eye space
    vec4 posV = V * posW;
    EPos = posV.xyz;

    // Output clipping-space position
    gl_Position = P * posV;

    // Pass through UVs
    vTexCoord = vertTex;

    mat3 normalMatrix = transpose(inverse(mat3(M)));
    fragNor = normalize(normalMatrix * vertNor);
}