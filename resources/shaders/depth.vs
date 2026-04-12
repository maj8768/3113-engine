#version 410

layout(location = 0) in vec3 vertexPosition;

uniform mat4 uLightSpaceMatrix;

out vec3 fragPos;

void main() {
    fragPos = vertexPosition;
    gl_Position = uLightSpaceMatrix * vec4(vertexPosition, 1.0);
}
