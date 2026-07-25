#version 330

// NOTE: 330 to pair with raylib's default vertex shader (LoadShader(0, ...)).

// =============================================================================
// glowbb.fs — soft depth-faded billboard light corona.
//
// A radial additive falloff (the glow shape) multiplied by a SOFT-PARTICLE factor:
// the fragment's own camera distance is compared to the scene depth at that pixel,
// so the glow fades smoothly as it approaches/enters a surface (no hard clip) and
// is hidden behind walls, while still reading as a full 3D halo in open space.
// =============================================================================

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec3 uGlowColor;       // peak brightness (light colour; may exceed 1)
uniform sampler2D uSceneDepth; // camera-view scene depth
uniform vec2 uScreenSize;      // framebuffer size in pixels
uniform float uProjA;          // perspective depth coeff A (from projMtx44)
uniform float uProjB;          // perspective depth coeff B
uniform float uFadeDist;       // world distance over which the glow fades at surfaces

// window depth [0,1] -> distance from the camera in world units, for this projection
float linearDist(float dwin) {
    float ndc = dwin * 2.0 - 1.0;
    return uProjB / (ndc + uProjA);
}

void main() {
    float d = distance(fragTexCoord, vec2(0.5));
    float x = clamp(1.0 - d * 2.0, 0.0, 1.0);
    vec3 core = uGlowColor * x * x;

    float sd = texture(uSceneDepth, gl_FragCoord.xy / uScreenSize).r;
    float sceneDist = (sd >= 0.9999) ? 1e30 : linearDist(sd); // cleared/background = far
    float glowDist  = linearDist(gl_FragCoord.z);

    // 0 when a surface is in front of the glow (occluded / behind a wall), ramping
    // to 1 as the glow sits in front of the surface by uFadeDist.
    float fade = clamp((sceneDist - glowDist) / uFadeDist, 0.0, 1.0);

    finalColor = vec4(core * fade, 1.0);
}
