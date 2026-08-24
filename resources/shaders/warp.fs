#version 330

// NOTE: 330 to pair with raylib's default vertex shader (LoadShader(0, ...)).

// =============================================================================
// warp.fs — a genuinely 3D heat/pressure SHOCKWAVE. The scene is captured to a
// colour + depth texture; this full-screen pass reconstructs each pixel's WORLD
// position from its depth, measures its true 3D distance to the blast origin, and
// refracts only the pixels whose world distance is within a thin band of the
// sphere's current radius. So the distortion lies ON the world surfaces the
// expanding sphere is passing through (a ring sweeping across the ground, climbing
// walls) instead of a flat ripple centred on the screen.
// =============================================================================

uniform sampler2D uColor;      // captured scene colour (bottom-up)
uniform sampler2D uDepth;      // captured scene depth (window depth [0,1])
uniform vec2  uResolution;     // framebuffer size, px
uniform float uTime;

// depth -> world reconstruction (camera ray + linear eye distance)
uniform float uProjA;          // perspective depth coeff A
uniform float uProjB;          // perspective depth coeff B
uniform vec3  uCamPos;
uniform vec3  uCamRight;       // screen +x  (unit)
uniform vec3  uCamUp;          // screen +y  (unit)
uniform vec3  uCamFwd;         // view forward (unit)
uniform float uTanHalf;        // tan(fov/2)
uniform float uAspect;

uniform int   uCount;
uniform vec3  uOrigin[4];      // blast centre, world
uniform float uRadius[4];      // shell radius, world units
uniform float uThickness[4];   // shell band half-width, world units
uniform float uStrength[4];    // peak screen refraction offset, px
uniform vec2  uCenter[4];      // blast centre projected to screen, px (radial direction)

out vec4 finalColor;

const float TAU = 6.28318530718;

// window depth [0,1] -> distance from the camera along the view axis (world units)
float linearDist(float dwin) {
    float ndc = dwin * 2.0 - 1.0;
    return uProjB / (ndc + uProjA);
}

void main() {
    vec2 frag = gl_FragCoord.xy;
    vec2 uv = frag / uResolution;

    // Reconstruct this pixel's world position from the captured depth.
    float sd = texture(uDepth, uv).r;
    float eyeZ = (sd >= 0.9999) ? 1e30 : linearDist(sd);
    vec2 ndc = uv * 2.0 - 1.0;
    // ray has unit forward component, so camPos + ray*eyeZ lands at forward-distance eyeZ.
    vec3 ray = uCamFwd + uCamRight * (ndc.x * uTanHalf * uAspect) + uCamUp * (ndc.y * uTanHalf);
    vec3 world = uCamPos + ray * eyeZ;

    vec2 disp = vec2(0.0);
    for (int i = 0; i < uCount; i++) {
        float wd   = distance(world, uOrigin[i]);   // TRUE 3D distance to the blast
        float edge = wd - uRadius[i];               // signed distance to the shell surface
        float th   = max(uThickness[i], 0.001);
        float band = 1.0 - smoothstep(0.0, th, abs(edge));
        if (band <= 0.0) continue;

        // Refract along the screen-radial direction from the blast's projected centre.
        vec2 d = frag - uCenter[i];
        float dl = length(d);
        vec2 dir = (dl > 1.0) ? d / dl : vec2(0.0);

        // Turbulent ripple across the band; the leading (outer) edge hits hardest.
        float ripple = sin(edge * (TAU / th) - uTime * 12.0)
                     + 0.5 * sin(edge * (2.5 * TAU / th) + uTime * 6.0);
        float lead = 1.0 + 0.6 * smoothstep(-th, 0.0, edge);
        disp += dir * (band * lead * ripple * uStrength[i]);
    }

    vec2 s = clamp(uv + disp / uResolution, vec2(0.001), vec2(0.999));
    finalColor = vec4(texture(uColor, s).rgb, 1.0);
}
