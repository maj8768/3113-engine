#version 410

in vec4 vColor;
in vec3 vNormal;
in vec2 TexCoord;
in vec3 fragPos;

uniform vec3  uLightPos;
uniform vec4  uLightColor;
uniform float uAmbient;
uniform sampler2D uTexo;

out vec4 fragColor;

void main() {
    vec3 n = normalize(vNormal);
    vec3 toLight = normalize(uLightPos - fragPos);
    float diffuse = max(dot(n, toLight), 0.0);

    vec3 lit = (uAmbient + diffuse) * uLightColor.rgb;
    vec4 texoColor = texture(uTexo, TexCoord);

    fragColor = vec4(lit, 1.0) * texoColor; // set 1.0 to vColor.a for vCo
}