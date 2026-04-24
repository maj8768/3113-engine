#version 410

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;

uniform mat4 uLightSpaceMatrix;
uniform mat4 uModel;

out vec2 TexCoord;

void main() {
    TexCoord = vertexTexCoord;
    gl_Position = uLightSpaceMatrix * uModel * vec4(vertexPosition, 1.0);
}
