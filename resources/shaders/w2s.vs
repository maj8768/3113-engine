#version 410

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;
layout(location = 3) in vec4 vertexColor;

out vec4 vColor;
out vec3 vNormal;
out vec2 TexCoord;
out vec4 fragPosLightSpace;
out vec3 vWorldPos;

uniform mat4 uVP;
uniform mat4 uModel;
uniform mat4 uLightSpaceMatrix;

void main() {
    vColor = vertexColor;
    TexCoord = vertexTexCoord;
    vec4 worldPos = uModel * vec4(vertexPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(mat3(uModel) * vertexNormal);
    fragPosLightSpace = uLightSpaceMatrix * worldPos;
    gl_Position = uVP * worldPos;
}
