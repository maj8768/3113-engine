#version 410

layout(location = 0) in vec3 vertexPosition;

uniform mat4 uLightSpaceMatrix;

void main() {
    gl_Position = uLightSpaceMatrix * vec4(vertexPosition, 1.0);
}
