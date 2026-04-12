#version 410

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;
layout(location = 3) in vec4 vertexColor;

out vec4 vColor;
out vec3 vNormal;
out vec2 TexCoord;
out vec3 fragPos;

uniform mat4 uVP;

void main() {
    vColor = vertexColor;
    vNormal = normalize(vertexNormal);
    TexCoord = vertexTexCoord;
    fragPos = vertexPosition;
    gl_Position = uVP * vec4(vertexPosition, 1.0);
}