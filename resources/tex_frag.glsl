#version 330 core

uniform sampler2D Texture0;
uniform vec3 lightPos;

in vec2 vTexCoord;
in vec3 fragNor;
in vec3 EPos;

out vec4 color;

void main()
{
    vec3 texColor = texture(Texture0, vTexCoord).rgb;

    vec3 normal = normalize(fragNor);
    vec3 light = normalize(lightPos - EPos);

    float dC = max(dot(normal, light), 0.0);

    vec3 final = texColor * dC;

    color = vec4(final, 1.0);
}