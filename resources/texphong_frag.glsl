#version 330 core

in vec3 vPos;
in vec3 vNormal;
in vec2 vTex;

uniform vec3 lightPos;
uniform vec3 camPos;

uniform sampler2D texture0;

out vec4 FragColor;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPos - vPos);
    vec3 V = normalize(camPos - vPos);
    vec3 R = reflect(-L, N);

    // Phong components
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(R, V), 0.0), 32.0);

    vec3 ambient = 0.25 * texture(texture0, vTex).rgb;
    vec3 diffuse = diff * texture(texture0, vTex).rgb;
    vec3 specular = spec * vec3(0.3);

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}