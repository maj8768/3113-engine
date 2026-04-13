#version 410

in vec4 vColor;
in vec3 vNormal;
in vec2 TexCoord;
in vec4 fragPosLightSpace;

uniform vec3  uLightDir;
uniform vec4  uLightColor;
uniform float uAmbient;
uniform sampler2D uTexo;
uniform sampler2D uShadowMap;
uniform float fadeTo;

out vec4 fragColor;

// https://noino.substack.com/p/raylib-graphics-shading
// https://renderdiagrams.org/2024/12/18/shadowmap-bias/
// https://ndotl.wordpress.com/2014/12/19/notes-on-shadow-bias/
float ShadowCalculation(float ndotl)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z < 0.0 || projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float sampleDepth = texture(uShadowMap, projCoords.xy).r;
    
    // https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing

    float bias = max(0.001, 0.0005 * (1.0 - ndotl));
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));

    vec2 offset = vec2(0.0);
    vec2 fracPos = fract(gl_FragCoord.xy * 0.5);
    offset.x = (fracPos.x > 0.25) ? 1.0 : 0.0;
    offset.y = (fracPos.y > 0.25) ? 1.0 : 0.0;
    offset.y = mod(offset.y + offset.x, 2.0);

    vec2 taps[4] = vec2[](
        offset + vec2(-1.5,  0.5),
        offset + vec2( 0.5,  0.5),
        offset + vec2(-1.5, -1.5),
        offset + vec2( 0.5, -1.5)
    );

    float sum = 0.0;
    for (int i = 0; i < 4; i++) {
        vec2 uv = clamp(projCoords.xy + taps[i] * texelSize, 0.0, 1.0);
        float sampleDepth = texture(uShadowMap, uv).r;
        sum += (currentDepth - bias > sampleDepth) ? 1.0 : 0.0;
    }

    return sum * 0.25;
}

void main() {
    vec3 n = normalize(vNormal);
    vec3 toLight = normalize(-uLightDir);

    float diffuse = max(dot(n, toLight), 0.0);
    float ndotl = max(dot(n, toLight), 0.0);
    float shadow = ShadowCalculation(ndotl);
    vec3 lit = (uAmbient + (1.0 - shadow) * diffuse) * uLightColor.rgb;
    fragColor = vec4(lit, 1.0) * texture(uTexo, TexCoord) * fadeTo;
}
