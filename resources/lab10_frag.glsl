#version 330 core

uniform sampler2D alphaTexture;

in vec4 partCol;

out vec4 outColor;

void main()
{
    float texAlpha = texture(alphaTexture, gl_PointCoord).r;

    // use RGB from particle, alpha from both lifetime and texture
    float finalAlpha = partCol.a * texAlpha;
    outColor = vec4(partCol.rgb, finalAlpha);
}
