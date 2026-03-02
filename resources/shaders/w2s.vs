#version 430
layout(location = 0) in vec3 vertexPosition;
layout(location = 2) in vec3 vertexNormal;
layout(location = 3) in vec4 vertexColor;
out vec4 vColor;
out vec3 vWorldPos;
void main()
{
    vColor    = vertexColor;
    vWorldPos = vertexNormal; // world pos packed here
    gl_Position = vec4(vertexPosition, 1.0); // already NDC
}