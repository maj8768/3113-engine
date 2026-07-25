#version 330

// NOTE: 330 (not 410 like w2s) — this shader is paired with raylib's DEFAULT
// vertex shader via LoadShader(0, ...), which is #version 330, so they link.

// =============================================================================
// bloom.fs — SMOOTH circular per-object variable-radius blur of the EMISSIVE buffer.
//
// texture0 holds the "emissive only" render:
//   rgb = that object's emissive color * amount * bloom BRIGHTNESS
//   a   = that object's bloom LENGTH projected to texels (how far it should spread)
//
// The kernel is a golden-angle (sunflower / Vogel) disc so the glow is radially
// symmetric. To avoid the stepped/blocky look of a fixed low-sample pattern:
//   * many samples,
//   * a per-pixel rotation of the spiral (decorrelates the pattern -> smooth,
//     not banded), and
//   * a smoothstep radial falloff.
// Each tap only counts within its own length, preserving per-object brightness
// and length. Drawn ADDITIVELY over the scene.
// =============================================================================

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;   // emissive-only buffer (bilinear filtered)
uniform vec4 colDiffuse;      // raylib tint (unused)

uniform vec2  uResolution;    // buffer size in pixels
uniform float uIntensity;     // master bloom brightness (tune this)
uniform float uSpread;        // max blur radius in texels (disc extent)

out vec4 finalColor;

const int   N      = 128;         // disc samples (fixed cost, independent of radius)
const float GOLDEN = 2.39996323;  // golden angle in radians (evenly fills the disc)

// Interleaved gradient noise — a spatially smooth per-pixel value in [0,1).
float ign(vec2 p) {
    return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715))));
}

void main() {
    vec2 texel = 1.0 / uResolution;
    float jitter = ign(gl_FragCoord.xy) * 6.28318530718; // per-pixel spiral rotation

    vec3 sum = texture(texture0, fragTexCoord).rgb; // center
    float wsum = 1.0;

    for (int k = 0; k < N; k++) {
        float fk = float(k) + 0.5;
        float r  = sqrt(fk / float(N)) * uSpread;   // radius in texels (even area distribution)
        float a  = fk * GOLDEN + jitter;            // spiral angle, rotated per pixel
        vec2  off = vec2(cos(a), sin(a)) * r * texel;

        vec4 s = texture(texture0, fragTexCoord + off);
        float sp = s.a * uSpread;                   // this tap's per-object length in texels
        if (sp <= 0.0 || r > sp) continue;          // outside that object's reach

        float t = r / sp;
        float w = smoothstep(1.0, 0.0, t);          // smooth radial falloff, 0 at the edge
        sum  += s.rgb * w;
        wsum += w;
    }

    finalColor = vec4(sum / wsum * uIntensity, 1.0);
}
