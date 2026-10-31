#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexGlowColor;

uniform mat4 uVP;

out vec2 fragTexCoord;
out vec3 vGlowColor;

void main() {
    fragTexCoord = vertexTexCoord;
    vGlowColor = vertexGlowColor;
    gl_Position = uVP * vec4(vertexPosition, 1.0);
}
