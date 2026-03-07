#version 410

in vec4 vColor;
in vec3 vWorldPos;

uniform vec3  uLightPos;
uniform vec3  uLightDir;
uniform vec4  uLightColor;
uniform float uAmbient;
uniform vec3  uNormal;

out vec4 fragColor;

void main() {
    vec3 n = normalize(vWorldPos); 
    vec3  toLight  = normalize(uLightPos - vWorldPos);
    float diffuse  = max(dot(n, toLight), 0.0);
    float dist     = length(uLightPos - vWorldPos);
    float atten    = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
    vec3  lit      = (uAmbient + diffuse * atten) * uLightColor.rgb * vColor.rgb;
    fragColor      = vec4(lit, vColor.a);
}
