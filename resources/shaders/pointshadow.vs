#version 410

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;

uniform mat4 uLightSpaceMatrix;
uniform mat4 uModel;

out vec2 TexCoord;
out vec3 vWorldPos;

void main() {
    vec4 worldPos = uModel * vec4(vertexPosition, 1.0);
    TexCoord = vertexTexCoord;
    vWorldPos = worldPos.xyz;
    gl_Position = uLightSpaceMatrix * worldPos;
}
