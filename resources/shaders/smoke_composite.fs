#version 330

uniform sampler2D uSmoke;
uniform sampler2D uOpaqueDepth;
uniform vec2 uDepthCoefficients;

out vec4 finalColor;

float linearDepth(float depth) {
    if (depth >= 1.0) return 1e20;
    return uDepthCoefficients.y / min(depth * 2.0 - 1.0 + uDepthCoefficients.x, -1e-7);
}

void main() {
    vec2 fullSize = vec2(textureSize(uOpaqueDepth, 0));
    ivec2 lowSize = textureSize(uSmoke, 0);
    vec2 uv = gl_FragCoord.xy / fullSize;
    vec2 lowPosition = uv * vec2(lowSize) - 0.5;
    ivec2 base = ivec2(floor(lowPosition));
    vec2 fraction = fract(lowPosition);
    ivec2 pixels[4];
    vec4 smoke[4];
    float coverage = 0.0;
    for (int index = 0; index < 4; index++) {
        pixels[index] = clamp(base + ivec2(index % 2, index / 2), ivec2(0), lowSize - 1);
        smoke[index] = texelFetch(uSmoke, pixels[index], 0);
        coverage += smoke[index].a;
    }
    // Empty smoke regions need no depth reads, exponentials, or framebuffer blending.
    if (coverage <= 0.0) discard;
    float sceneDepth = linearDepth(texture(uOpaqueDepth, uv).r);
    vec4 sum = vec4(0.0);
    float totalWeight = 0.0;
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            int index = y * 2 + x;
            ivec2 pixel = pixels[index];
            vec2 sampleUV = (vec2(pixel) + 0.5) / vec2(lowSize);
            float guideDepth = linearDepth(texture(uOpaqueDepth, sampleUV).r);
            float tolerance = 0.1 + 0.02 * min(sceneDepth, guideDepth);
            float depthWeight = exp(-abs(sceneDepth - guideDepth) / tolerance);
            vec2 spatial = mix(vec2(1.0) - fraction, fraction, vec2(x, y));
            float weight = spatial.x * spatial.y * depthWeight;
            sum += smoke[index] * weight;
            totalWeight += weight;
        }
    }
    // No compatible low-res sample: preserve the foreground object's sharp edge.
    finalColor = totalWeight > 1e-6 ? sum / totalWeight : vec4(0.0);
}
