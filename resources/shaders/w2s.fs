#version 410

// =============================================================================
// w2s.fs  ("world-to-screen" fragment shader) — the main lit/shadowed pass.
//
// Runs once per pixel of every 3D object drawn via Draw3DGPU. It:
//   1. computes lighting (sun diffuse + ambient + point lights),
//   2. applies point-light cubemap shadows (3x3 PCF),
//   3. samples the object texture,
//   4. stacks several full-screen-style COLOR OVERLAYS on top, and
//   5. applies distance FOG.
//
// If you are hunting for the FOG and the COLOR OVERLAY/TINT, jump to main():
//   * FOG            -> the "DISTANCE FOG" block near the bottom of main().
//   * COLOR OVERLAYS -> the warm uLightColor tint (in the lighting), the
//                       `fadeTo` multiply, drunkEffect(), and
//                       drankUrgencyEffect() — all marked "COLOR OVERLAY" below.
// =============================================================================

// ---- Inputs interpolated from the vertex shader (per-fragment) --------------
in vec4 vColor;            // vertex color (unused by the final output here)
in vec3 vNormal;           // surface normal (may be zero on some platforms)
in vec2 TexCoord;          // texture UV
in vec3 vWorldPos;         // this fragment's world-space position (used for fog, point lights, shadows)

// ---- Lighting / sun uniforms ------------------------------------------------
uniform vec3 uLightDir;    // (legacy) old directional sun vector — no longer used for shading
uniform vec3 uLightPos;    // POINT-LIGHT SUN position (world space); shading direction is derived from this per-pixel
uniform float uLightRange; // POINT-LIGHT SUN falloff distance: brightness fades to 0 at this range
uniform vec4 uLightColor;  // SUN COLOR — set warm/orange in main.cpp ({1.0, 0.72, 0.2}).
                           // This is the biggest source of the overall COLOR TINT: it
                           // multiplies the lit result AND is reused as the fog color.
uniform float uAmbient;    // ambient light amount (also scales the fog color)
uniform sampler2D uTexo;   // this object's diffuse texture
uniform vec4 uEmissive;    // per-object emissive: rgb = glow color, a = amount (0..1)
uniform int uEmissiveOnly; // BLOOM: when !=0, output only the emissive term (for the bloom source buffer)
uniform vec2 uBloom;       // BLOOM per-object: x = brightness, y = length in WORLD units
uniform float uBloomFocal; // BLOOM: (bufferHeight/2)/tan(fovY/2) — converts world length to screen texels
uniform float uBloomMax;   // BLOOM: max blur radius in buffer texels (normalizes the stored alpha)

// ---- Shadow uniforms --------------------------------------------------------
uniform samplerCube uShadowMap; // radial depth around uLightPos, normalized by uLightRange
uniform int uShadowsEnabled;       // 0 = skip shadows entirely

// ---- Screen-space effect uniforms ------------------------------------------
uniform float fadeTo;        // COLOR OVERLAY: global fade multiplier (1 = normal, 0 = black). Used for level fades.
uniform float uTime;         // seconds, drives animated drunk effect
uniform float uDrunkenness;  // COLOR OVERLAY: 0..1 strength of the drunk distortion/tint
uniform float uDrankUrgency; // COLOR OVERLAY: 0..1 strength of the vignette+desaturate ("need a drank") effect
uniform vec3 uCamPos;        // camera world position (used for fog)
uniform vec2 uResolution;    // viewport size in pixels (for screen-space UVs)

// ---- Point lights -----------------------------------------------------------
#define MAX_POINT_LIGHTS 32
uniform vec3 uPointPos[MAX_POINT_LIGHTS];    // world positions
uniform vec3 uPointColor[MAX_POINT_LIGHTS];  // colors
uniform float uPointRadius[MAX_POINT_LIGHTS];// falloff radius
// Per-light AMBIENT: a flat, normal-independent share of the light's colour, added
// alongside the N.L term and attenuated the same way. 0 = pure diffuse (every light
// before this existed). Lets an effect wash nearby geometry evenly instead of only
// lighting faces that happen to point at it.
uniform float uPointAmbient[MAX_POINT_LIGHTS];
uniform int uPointCount;                     // how many are active

// ---- Beam (segment) lights --------------------------------------------------
// Each is a line light: a whole segment emits, not a point. Per fragment we light from
// the CLOSEST point on the segment, so one light covers the entire beam.
#define MAX_BEAM_LIGHTS 4
uniform vec3 uBeamStart[MAX_BEAM_LIGHTS];
uniform vec3 uBeamEnd[MAX_BEAM_LIGHTS];
uniform vec3 uBeamColor[MAX_BEAM_LIGHTS];
uniform float uBeamRadius[MAX_BEAM_LIGHTS];
uniform float uBeamAmbient[MAX_BEAM_LIGHTS]; // see uPointAmbient
uniform int uBeamCount;

out vec4 fragColor; // final pixel color written to the framebuffer

// Return the actual texel-center ray used by nearest cubemap sampling. The
// receiver-plane comparison must use this ray too, especially on sloped faces.
vec3 shadowTexelDirection(vec3 direction, float resolution) {
    vec3 major = abs(direction);
    vec3 forward, right, up;
    if (major.x >= major.y && major.x >= major.z) {
        float side = sign(direction.x);
        forward = vec3(side, 0.0, 0.0);
        right = vec3(0.0, 0.0, -side);
        up = vec3(0.0, -1.0, 0.0);
    } else if (major.y >= major.z) {
        float side = sign(direction.y);
        forward = vec3(0.0, side, 0.0);
        right = vec3(1.0, 0.0, 0.0);
        up = vec3(0.0, 0.0, side);
    } else {
        float side = sign(direction.z);
        forward = vec3(0.0, 0.0, side);
        right = vec3(side, 0.0, 0.0);
        up = vec3(0.0, -1.0, 0.0);
    }
    vec2 plane = vec2(dot(direction, right), dot(direction, up)) / dot(direction, forward);
    vec2 pixel = clamp(floor((plane * 0.5 + 0.5) * resolution), vec2(0.0), vec2(resolution - 1.0));
    vec2 center = (pixel + 0.5) * (2.0 / resolution) - 1.0;
    return normalize(forward + right * center.x + up * center.y);
}

// Small angular PCF footprint, independent of camera position. Depth comparisons
// use world units so bias does not grow with the shadow projection's depth range.
float ShadowCalculation(vec3 normal, float ndotl) {
    vec3 fromLight = vWorldPos - uLightPos;
    float distanceToLight = length(fromLight);
    if (distanceToLight < 1e-4 || distanceToLight >= uLightRange) return 0.0;

    vec3 direction = fromLight / distanceToLight;
    vec3 axis = abs(direction.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(direction, axis));
    vec3 up = cross(right, direction);
    float resolution = float(textureSize(uShadowMap, 0).x);
    float angularTexel = 2.0 / resolution;
    float bias = max(0.015 * (1.0 - ndotl), 0.003);
    float planeDistance = dot(normal, fromLight);

    float shadow = 0.0;
    float totalWeight = 0.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec3 sampleDirection = shadowTexelDirection(direction +
                (right * float(x) + up * float(y)) * angularTexel, resolution);
            // Compare each tap against this fragment's receiver plane. This keeps
            // sloping surfaces from shadowing themselves without a large depth bias.
            float denominator = dot(normal, sampleDirection);
            float receiverDepth = distanceToLight;
            if (abs(denominator) > 0.1) receiverDepth = planeDistance / denominator;
            float storedDepth = texture(uShadowMap, sampleDirection).r * uLightRange;
            float weight = float((2 - abs(x)) * (2 - abs(y)));
            shadow += weight * float(receiverDepth - bias > storedDepth);
            totalWeight += weight;
        }
    }
    return shadow / totalWeight;
}

// =============================================================================
// drunkEffect — COLOR OVERLAY #A (screen-space, animated).
// A full-screen post-style distortion applied to the already-shaded pixel when
// uDrunkenness > 0. Purely a look/feel overlay; does NOT affect geometry.
// Layers, all scaled by `drunkenness`:
//   * chromatic aberration (red/blue pushed apart toward screen edges),
//   * a "squint" smear at high drunkenness,
//   * a vibrating row/vertical wobble over time,
//   * pulsing god-ray spokes near the screen edge.
// screenUV = this pixel's [0,1] position on screen; time = uTime.
// =============================================================================
vec4 drunkEffect(vec4 color, vec2 screenUV, float time, float drunkenness) {
    drunkenness = clamp(drunkenness, 0.0, 1.0);
    if (drunkenness <= 0.0) return color; // early out when sober -> no overlay

    vec2 uv = screenUV;
    vec2 center = vec2(0.5, 0.5);
    vec2 toEdge = uv - center;   // vector from screen center to this pixel
    float dist = length(toEdge); // distance from center (drives edge effects)

    // squintT: only ramps in during the upper half of the drunkenness range.
    float squintT = clamp((drunkenness - 0.5) / 0.5, 0.0, 1.0);

    // --- Chromatic aberration: shove R one way and B the other, more toward edges.
    float aberrStr = drunkenness * 0.018;
    vec2 aberrDir = normalize(toEdge + vec2(1e-5)) * aberrStr;
    float rBoost = dot(toEdge, aberrDir) * 6.0;
    float bBoost = dot(toEdge, -aberrDir) * 6.0;
    color.r = clamp(color.r + rBoost * drunkenness, 0.0, 1.0);
    color.b = clamp(color.b + bBoost * drunkenness, 0.0, 1.0);

    // --- Squint smear: adds a luminance-gradient-based streak at high drunkenness.
    float sqBlur = squintT * 0.12;
    vec2 sqDir = normalize(toEdge + vec2(1e-5)) * sqBlur;
    float sqRBoost = dot(toEdge, sqDir) * 8.0;
    float sqBBoost = dot(toEdge, -sqDir) * 8.0;
    float sqLum = color.r * 0.299 + color.g * 0.587 + color.b * 0.114; // perceptual luma
    vec3 sqGrad = vec3(dFdx(sqLum), dFdy(sqLum), 0.0);                  // screen-space luma gradient
    float smear = (sqGrad.x + sqGrad.y) * squintT * 6.0;
    color.r = clamp(color.r + sqRBoost * squintT + smear, 0.0, 1.0);
    color.g = clamp(color.g + smear * 0.5, 0.0, 1.0);
    color.b = clamp(color.b + sqBBoost * squintT + smear, 0.0, 1.0);

    // --- Vibration: time-based wobble that shifts color along screen gradients.
    float vibAmp = drunkenness * 0.03;
    float vibRate = 18.0;
    float rowShift = sin(time * vibRate + uv.y * 25.0) * vibAmp;      // per-row horizontal jitter
    float vertShift = sin(time * vibRate * 0.7 + 1.3) * vibAmp * 0.5; // whole-frame vertical jitter
    float hGrad = dFdx(color.r + color.g + color.b);
    float vGrad = dFdy(color.r + color.g + color.b);
    color.rgb += rowShift * hGrad * 40.0;
    color.rgb += vertShift * vGrad * 40.0;
    color.rgb = clamp(color.rgb, 0.0, 1.0);

    // --- God-ray spokes: pulsing bright rays that fade in near the screen edge.
    float rayLength = drunkenness * 0.45;
    float edgeFade = smoothstep(0.5 - rayLength, 0.5, dist); // only near the edge
    float angle = atan(toEdge.y, toEdge.x);
    float raySpokes = abs(sin(angle * 5.0 + time * 0.7));    // 5 rotating spokes
    float pulse = sin(time * 2.5) * 0.5 + 0.5;               // brightness pulse
    float ray = edgeFade * raySpokes * pulse * drunkenness * 0.5;
    color.rgb = clamp(color.rgb + ray, 0.0, 1.5);            // note: allowed to bloom >1

    return color;
}

// =============================================================================
// drankUrgencyEffect — COLOR OVERLAY #B (screen-space vignette + desaturation).
// Driven by uDrankUrgency (0..1). As urgency rises the screen desaturates toward
// gray and a dark vignette closes in from the edges; at urgency >= 1 the pixel
// goes fully black (screen blackout). This is a UI/state overlay, not lighting.
// =============================================================================
vec4 drankUrgencyEffect(vec4 color, vec2 screenUV, float urgency) {
    if (urgency <= 0.0) return color;                       // no effect
    if (urgency >= 1.0) return vec4(0.0, 0.0, 0.0, color.a);// full blackout

    // Aspect-corrected distance from screen center.
    vec2 centered = screenUV - vec2(0.5);
    centered.x *= uResolution.x / uResolution.y;
    float dist = length(centered);

    // Vignette ring shrinks toward the center as urgency grows.
    float outerEdge = mix(1.2, 0.02, urgency);
    float softness = max(0.01, outerEdge * 0.55);
    float vignette = smoothstep(outerEdge, outerEdge - softness, dist); // 1 in center -> 0 at edge

    // Desaturate toward luminance by `urgency`.
    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    vec3 desat = mix(color.rgb, vec3(gray), urgency);

    // Multiply by the vignette so edges darken to black.
    return vec4(desat * vignette, color.a);
}

// =============================================================================
// main — assembles the final pixel color, in order:
//   normal -> lighting -> shadows -> point lights -> texture ->
//   [fadeTo overlay] -> [drunk overlay] -> [FOG] -> [drank-urgency overlay]
// =============================================================================
void main() {
    // --- Surface normal -------------------------------------------------------
    // Use the interpolated vertex normal when valid; otherwise fall back to a
    // screen-space geometric normal (derivatives of world pos). Needed because
    // rlgl's batch renderer doesn't always wire normals on every platform (Mac).
    vec3 n;
    float nLen = dot(vNormal, vNormal);
    if (nLen > 0.001) {
        n = vNormal / sqrt(nLen);           // normalize the provided normal
    } else {
        vec3 dPdx = dFdx(vWorldPos);
        vec3 dPdy = dFdy(vWorldPos);
        n = normalize(cross(dPdx, dPdy));   // reconstruct a flat-face normal
    }
    // POINT-LIGHT SUN: compute the light direction and distance per-pixel from
    // the light's world position (uLightPos) instead of a fixed directional
    // vector. This per-pixel, position-based direction + distance falloff is what
    // makes it a point light rather than a directional sun.
    vec3 toLightVec = uLightPos - vWorldPos;
    float lightDist = length(toLightVec);
    vec3 toLight = toLightVec / max(lightDist, 1e-4); // normalized dir: surface -> light

    // --- Sun lighting + shadows ----------------------------------------------
    float diffuse = max(dot(n, toLight), 0.0);                    // Lambert term
    float atten = clamp(1.0 - lightDist / uLightRange, 0.0, 1.0); // point-light distance falloff (1 near, 0 at range)
    float ndotl = diffuse;
    float shadow = (uShadowsEnabled != 0) ? ShadowCalculation(n, ndotl) : 0.0; // 0 lit .. 1 shadowed
    // COLOR OVERLAY (base tint): ambient + attenuated shadowed-diffuse, all
    // multiplied by uLightColor. Because uLightColor is warm/orange, EVERYTHING
    // gets a warm cast here — the primary scene color tint before any post effects.
    vec3 lit = (uAmbient + (1.0 - shadow) * diffuse * atten) * uLightColor.rgb;

    // --- Point lights ---------------------------------------------------------
    // Add each active point light: linear-ish radial falloff, squared for a
    // softer edge, modulated by the surface normal facing the light.
    for (int i = 0; i < uPointCount; i++) {
        vec3 toPoint = uPointPos[i] - vWorldPos;
        float distanceSquared = dot(toPoint, toPoint);
        float radius = uPointRadius[i];
        if (radius <= 0.0 || distanceSquared >= radius * radius) continue;
        float dist = sqrt(distanceSquared);
        float atten = 1.0 - dist / radius; // 1 at light, 0 at radius
        atten *= atten;                                       // square -> softer falloff
        float diffuse = max(dot(n, toPoint) / max(dist, 1e-4), 0.0);
        lit += (uPointAmbient[i] + diffuse) * atten * uPointColor[i];
    }

    // --- Beam (segment) lights: light from the closest point on each segment, so the
    // whole beam emits with a single light (same radial falloff as point lights).
    for (int i = 0; i < uBeamCount; i++) {
        vec3 ab = uBeamEnd[i] - uBeamStart[i];
        float denom = max(dot(ab, ab), 1e-4);
        float s = clamp(dot(vWorldPos - uBeamStart[i], ab) / denom, 0.0, 1.0);
        vec3 toBeam = (uBeamStart[i] + s * ab) - vWorldPos; // closest point on the segment
        float distanceSquared = dot(toBeam, toBeam);
        float radius = uBeamRadius[i];
        if (radius <= 0.0 || distanceSquared >= radius * radius) continue;
        float dist = sqrt(distanceSquared);
        float atten = 1.0 - dist / radius;
        atten *= atten;
        float diffuse = max(dot(n, toBeam) / max(dist, 1e-4), 0.0);
        lit += (uBeamAmbient[i] + diffuse) * atten * uBeamColor[i];
    }

    // --- Texture --------------------------------------------------------------
    vec4 texSample = texture(uTexo, TexCoord);
    if (texSample.a < 0.5) discard; // alpha-cutout: skip mostly-transparent texels

    // BLOOM emissive pass: write the bloom SOURCE for this object.
    //   rgb = emissive color * amount * bloom brightness (uBloom.x)  -> halo brightness
    //   a   = PHYSICAL bloom length (uBloom.y, WORLD units) projected to screen
    //         texels, so the halo stays a constant WORLD size regardless of how
    //         far the camera is: texels = worldLength * focal / distanceToCamera.
    // Non-emissive surfaces have amount 0 -> rgb black -> contribute no bloom.
    if (uEmissiveOnly != 0) {
        float distCam = length(vWorldPos - uCamPos);
        float spreadTexels = uBloom.y * uBloomFocal / max(distCam, 0.001);
        float spread01 = clamp(spreadTexels / max(uBloomMax, 1.0), 0.0, 1.0);
        fragColor = vec4(uEmissive.rgb * uEmissive.a * uBloom.x, spread01);
        return;
    }

    // Base shaded color = lighting * texture.
    // COLOR OVERLAY: `* fadeTo` is a global multiply used for fade-to-black
    // transitions (fadeTo=1 normal, fadeTo=0 black).
    fragColor = vec4(lit, 1.0) * texSample * fadeTo;

    // EMISSIVE: per-object self-illumination added on top of the lit color, so it
    // glows regardless of lighting/shadow. uEmissive.rgb = color, uEmissive.a =
    // amount (0..1). * fadeTo so it still respects level fade-outs.
    fragColor.rgb += uEmissive.rgb * uEmissive.a * fadeTo;

    // Screen-space UV for the post overlays below.
    vec2 screenUV = gl_FragCoord.xy / uResolution;

    // COLOR OVERLAY #A: drunk distortion/tint (see drunkEffect above).
    fragColor = drunkEffect(fragColor, screenUV, uTime, uDrunkenness);

    // === DISTANCE FOG =========================================================
    // >>> THIS is the fog. <<<
    // Blend the pixel toward `fogColor` based on how far it is from the camera.
    //   fogDist   : world distance from camera to this fragment.
    //   fogFactor : 0 within 5 units, ramping linearly to 1 at 5+95 = 100 units.
    //   fogColor  : the AMBIENT-scaled SUN COLOR (uAmbient * uLightColor.rgb).
    //               Because uLightColor is warm, the fog (and thus the whole
    //               distance/horizon) reads as a warm orange haze. To retint or
    //               disable the fog, change fogColor or clamp fogFactor here.
    float fogDist = length(vWorldPos - uCamPos);
    float fogFactor = clamp((fogDist - 5.0) / 95.0, 0.0, 1.0);
    vec3 fogColor = uAmbient * uLightColor.rgb;
    // fragColor.rgb = mix(fragColor.rgb, fogColor, fogFactor);
    // ==========================================================================

    // COLOR OVERLAY #B: vignette + desaturate + blackout (see drankUrgencyEffect).
    fragColor = drankUrgencyEffect(fragColor, screenUV, uDrankUrgency);

    // Per-object opacity from the vertex-color alpha (Draw3DGPU passes it via rlColor4ub).
    // 1.0 for every normal object; a draw can pass alpha < 255 to fade out (heaven sword).
    fragColor.a *= vColor.a;
}
