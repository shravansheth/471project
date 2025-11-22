#version 330 core
out vec4 color;

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float MatShine;
uniform vec3 LightColor;

in vec3 fragNor;
in vec3 lightDir;
in vec3 EPos;

void main()
{
    vec3 normal = normalize(fragNor);
    vec3 light  = normalize(lightDir);

    float dC = max(dot(normal, light), 0.0);

    vec3 v = normalize(-EPos);
    vec3 h = normalize(light + v);

    float sC = 0.0;
    if (dC > 0.0) {
        sC = pow(max(dot(normal, h), 0.0), MatShine);
    }

    vec3 rgb = MatAmb + (dC * MatDif * LightColor) + (sC * MatSpec * LightColor);
    color = vec4(rgb, 1.0);
}