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

    vec3 N = normalize(fragNor);

    // Light and eye vectors in eye space
    vec3 L = normalize(lightPos - EPos);
    vec3 V = normalize(-EPos);
    vec3 R = reflect(-L, N);

    // Phong components
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(R, V), 0.0), 32.0);

    vec3 ambient = 0.25 * texColor;
    vec3 diffuse = diff * texColor;
    vec3 specular = spec * vec3(0.3);

    color = vec4(ambient + diffuse + specular, 1.0);
}