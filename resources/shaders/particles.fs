#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform int uSmokePass;
uniform sampler2D uOpaqueDepth;
uniform vec2 uSmokeResolution;
uniform vec2 uDepthCoefficients;

out vec4 finalColor;

void main() {
    vec4 sprite = texture(texture0, fragTexCoord);
    vec4 color = sprite * fragColor * colDiffuse;
    // These passes never write depth. A zero-alpha fragment contributes nothing
    // with either alpha or additive blending, so avoid its framebuffer blend.
    if (color.a <= 0.0) discard;
    if (uSmokePass != 0) {
        float sceneDepth = texture(uOpaqueDepth, gl_FragCoord.xy / uSmokeResolution).r;
        if (gl_FragCoord.z >= sceneDepth) discard;
        float sceneDistance = sceneDepth >= 1.0 ? 1e20 : uDepthCoefficients.y /
            min(sceneDepth * 2.0 - 1.0 + uDepthCoefficients.x, -1e-7);
        float smokeDistance = uDepthCoefficients.y /
            min(gl_FragCoord.z * 2.0 - 1.0 + uDepthCoefficients.x, -1e-7);
        color.a *= clamp((sceneDistance - smokeDistance) / 0.5, 0.0, 1.0);
        if (color.a <= 0.0) discard;
    }
    finalColor = color;
}
