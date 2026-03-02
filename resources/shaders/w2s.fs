#version 430
in vec4 vColor;
in vec3 vWorldPos;
out vec4 finalColor;

layout(location = 0) uniform vec4  uColor;
layout(location = 1) uniform vec3  uLightPos;
layout(location = 2) uniform vec4  uLightColor;
layout(location = 3) uniform float uAmbient;
layout(location = 4) uniform vec3  uNormal;

void main()
{
    vec3 norm     = normalize(uNormal);
    vec3 lightDir = normalize(uLightPos - vWorldPos);
    float diff    = max(dot(norm, lightDir), 0.0);

    float dist  = length(uLightPos - vWorldPos);
    float atten = 1.0 / (1.0 + 0.01*dist + 0.005*dist*dist);

    vec4 ambient = uAmbient * uColor * vColor;
    vec4 diffuse = diff * atten * uLightColor * uColor * vColor;
    finalColor = ambient + diffuse;
}