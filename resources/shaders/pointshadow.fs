#version 410

in vec2 TexCoord;
in vec3 vWorldPos;

uniform sampler2D uTexo;
uniform vec3 uLightPos;
uniform float uLightRange;

out vec4 fragColor;

void main() {
    if (texture(uTexo, TexCoord).a < 0.5) discard;
    float distanceToLight = length(vWorldPos - uLightPos);
    if (distanceToLight >= uLightRange) discard;
    // Store radial distance; every face uses the same comparison in world units.
    fragColor = vec4(distanceToLight / uLightRange, 0.0, 0.0, 1.0);
}
