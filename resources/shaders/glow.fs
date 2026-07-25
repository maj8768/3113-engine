#version 330

// NOTE: 330 to pair with raylib's default vertex shader (LoadShader(0, ...)).

// =============================================================================
// glow.fs — physical light corona / bloom (screen-space analytic falloff) with a
// straight PER-PIXEL depth test: a glow pixel is drawn only where the light is in
// FRONT of the scene geometry at that pixel. Where geometry is nearer than the
// light, that part of the corona is skipped.
// =============================================================================

uniform int   uGlowCount;
uniform vec2  uGlowCenter[8];  // screen pixels (GL bottom-left, matches gl_FragCoord)
uniform float uGlowRadius[8];  // screen pixels (world halo radius projected)
uniform vec3  uGlowColor[8];   // light colour (peak brightness)
uniform float uGlowDepth[8];   // each emitter's window depth [0,1]

uniform sampler2D uSceneDepth; // camera-view scene depth (emitters excluded)
uniform vec2  uScreenSize;     // pixels (actual framebuffer)
uniform float uProjA;          // perspective depth coeff A (from projMtx44)
uniform float uProjB;          // perspective depth coeff B
uniform float uDepthBias;      // world-units slack on the comparison

out vec4 finalColor;

// window depth [0,1] -> distance from the camera in world units
float linearDist(float dwin) {
    float ndc = dwin * 2.0 - 1.0;
    return uProjB / (ndc + uProjA);
}

void main() {
    vec2 frag = gl_FragCoord.xy;

    // Scene distance at THIS pixel (background/cleared = infinitely far).
    float sd = texture(uSceneDepth, frag / uScreenSize).r;
    float sceneDist = (sd >= 0.9999) ? 1e30 : linearDist(sd);

    vec3 c = vec3(0.0);
    for (int i = 0; i < uGlowCount; i++) {
        // Skip this pixel if the light is BEHIND the geometry here (nearer geometry).
        if (linearDist(uGlowDepth[i]) > sceneDist + uDepthBias) continue;

        float d = distance(frag, uGlowCenter[i]);
        float x = clamp(1.0 - d / max(uGlowRadius[i], 1.0), 0.0, 1.0);
        c += uGlowColor[i] * x * x; // smooth radial falloff; peak at d=0 = colour
    }
    finalColor = vec4(c, 1.0);
}
