#version 330

// NOTE: 330 to pair with raylib's default vertex shader (LoadShader(0, ...)).

// Solid (opaque) smoke puff. The round sprite's alpha is used as a coverage MASK,
// not for blending: fragments below the dissolve threshold are discarded so they
// write neither colour nor depth. The threshold is carried per-particle in the
// vertex-colour alpha (fragColor.a): ~0 = full solid disc, rising toward 1 as the
// particle ages, so the puff erodes away from its edges while staying opaque (no
// translucent glow). Because the kept fragments are opaque and write depth, they
// occlude the additive fire drawn afterwards.

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

out vec4 finalColor;

const float EDGE_SOFT = 0.35; // width of the feathered rim (in mask units)

void main() {
    float mask = texture(texture0, fragTexCoord).a; // 1 at centre -> 0 at edge
    float thr  = fragColor.a;                        // dissolve threshold
    if (mask <= thr) discard;                        // outside the puff: no colour, no depth
    // Soft feathered rim: opaque core, alpha fades out toward the edge.
    float alpha = smoothstep(thr, thr + EDGE_SOFT, mask);
    finalColor = vec4(fragColor.rgb, alpha);
}
