#version 410

in vec3 fragPos;

uniform vec3  uLightPos;
uniform float uFarPlane;

out vec4 fragColor;

void main() {
    float dist = length(fragPos - uLightPos) / uFarPlane;
    fragColor = vec4(dist, dist, dist, 1.0);
}
