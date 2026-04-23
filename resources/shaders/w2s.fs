#version 410

in vec4 vColor;
in vec3 vNormal;
in vec2 TexCoord;
in vec4 fragPosLightSpace;
in vec3 vWorldPos;

uniform vec3  uLightDir;
uniform vec4  uLightColor;
uniform float uAmbient;
uniform sampler2D uTexo;
uniform sampler2D uShadowMap;     // near cascade
uniform sampler2D uShadowMapFar;  // far cascade
uniform float fadeTo;
uniform float uTime;
uniform float uDrunkenness;
uniform int uShadowsEnabled;
uniform vec3  uCamPos;
uniform mat4  uLightSpaceMatrixFar;
uniform float uCascadeSplit;

out vec4 fragColor;

// Bilinear interpolation of the shadow comparison across 4 neighboring texels.
float sampleShadowBilinear(sampler2D sm, vec2 uv, float refDepth, vec2 ts) {
    vec2 f    = fract(uv / ts);
    vec2 base = floor(uv / ts) * ts;
    float s00 = float(refDepth > texture(sm, base).r);
    float s10 = float(refDepth > texture(sm, base + vec2(ts.x, 0.0)).r);
    float s01 = float(refDepth > texture(sm, base + vec2(0.0, ts.y)).r);
    float s11 = float(refDepth > texture(sm, base + ts).r);
    return mix(mix(s00, s10, f.x), mix(s01, s11, f.x), f.y);
}

float ShadowPCF(sampler2D sm, vec4 lightSpacePos, float ndotl, float spread)
{
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z < 0.0 || projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float bias      = max(0.02 * (1.0 - ndotl), 0.02);
    float refDepth  = projCoords.z - bias;
    vec2  texelSize = 1.0 / vec2(textureSize(sm, 0));

    float sum = 0.0, totalWeight = 0.0w;
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            float w  = exp(-float(x*x + y*y) * 0.2);   // wide Gaussian (0.2 = softer falloff)
            vec2  uv = projCoords.xy + vec2(float(x), float(y)) * texelSize * spread;
            sum += w * sampleShadowBilinear(sm, uv, refDepth, texelSize);
            totalWeight += w;
        }
    }
    return sum / totalWeight;
}

float ShadowCalculation(float ndotl)
{
    float fragDist = length(vWorldPos - uCamPos);

    // Blend near -> far over a short distance around CASCADE_SPLIT
    float blend = smoothstep(uCascadeSplit * 0.8, uCascadeSplit, fragDist);

    float shadowNear = ShadowPCF(uShadowMap,    fragPosLightSpace,                       ndotl, 4.0);
    vec4  farPos     = uLightSpaceMatrixFar * vec4(vWorldPos, 1.0);
    float shadowFar  = ShadowPCF(uShadowMapFar, farPos,                                  ndotl, 3.0);

    return mix(shadowNear, shadowFar, blend);
}


// screenUV: gl_FragCoord.xy / resolution (0..1)
// time: elapsed seconds
// drunkenness: 0..1
vec4 drunkEffect(vec4 color, vec2 screenUV, float time, float drunkenness) {
    drunkenness = clamp(drunkenness, 0.0, 1.0);
    if (drunkenness <= 0.0) return color;

    vec2 uv     = screenUV;
    vec2 center = vec2(0.5, 0.5);
    vec2 toEdge = uv - center;          // points outward from center
    float dist  = length(toEdge);

    // Squint factor — used by blur section, fades in above 0.5
    float squintT = clamp((drunkenness - 0.5) / 0.5, 0.0, 1.0);

    // --- 1. Chromatic aberration blur ---
    float aberrStr = drunkenness * 0.018;
    vec2 aberrDir  = normalize(toEdge + vec2(1e-5)) * aberrStr;
    float rBoost   = dot(toEdge,  aberrDir) * 6.0;
    float bBoost   = dot(toEdge, -aberrDir) * 6.0;
    color.r = clamp(color.r + rBoost * drunkenness, 0.0, 1.0);
    color.b = clamp(color.b + bBoost * drunkenness, 0.0, 1.0);

    // Squint blur
    float sqBlur    = squintT * 0.12;
    vec2 sqDir      = normalize(toEdge + vec2(1e-5)) * sqBlur;
    float sqRBoost  = dot(toEdge,  sqDir) * 8.0;
    float sqBBoost  = dot(toEdge, -sqDir) * 8.0;
    float sqLum     = color.r * 0.299 + color.g * 0.587 + color.b * 0.114;
    vec3 sqGrad     = vec3(dFdx(sqLum), dFdy(sqLum), 0.0);
    float smear     = (sqGrad.x + sqGrad.y) * squintT * 6.0;
    color.r = clamp(color.r + sqRBoost * squintT + smear, 0.0, 1.0);
    color.g = clamp(color.g + smear * 0.5,                0.0, 1.0);
    color.b = clamp(color.b + sqBBoost * squintT + smear, 0.0, 1.0);

    // --- 4. Vibration ---
    float vibAmp  = drunkenness * 0.03;
    float vibRate = 18.0;
    float rowShift = sin(time * vibRate + uv.y * 25.0) * vibAmp;
    float vertShift = sin(time * vibRate * 0.7 + 1.3) * vibAmp * 0.5;
    float hGrad = dFdx(color.r + color.g + color.b);
    float vGrad = dFdy(color.r + color.g + color.b);
    color.rgb += rowShift  * hGrad * 40.0;
    color.rgb += vertShift * vGrad * 40.0;
    color.rgb  = clamp(color.rgb, 0.0, 1.0);

    // --- 2. Edge ray effect ---
    float rayLength = drunkenness * 0.45;
    float edgeFade  = smoothstep(0.5 - rayLength, 0.5, dist);
    float angle     = atan(toEdge.y, toEdge.x);
    float raySpokes = abs(sin(angle * 5.0 + time * 0.7));
    float pulse     = sin(time * 2.5) * 0.5 + 0.5;
    float ray       = edgeFade * raySpokes * pulse * drunkenness * 0.5;
    color.rgb       = clamp(color.rgb + ray, 0.0, 1.5);

    return color;
}

void main() {
    vec3 n = normalize(vNormal);
    vec3 toLight = normalize(-uLightDir);

    float diffuse = max(dot(n, toLight), 0.0);
    float ndotl = diffuse;
    float shadow = (uShadowsEnabled != 0) ? ShadowCalculation(ndotl) : 0.0;
    vec3 lit = (uAmbient + (1.0 - shadow) * diffuse) * uLightColor.rgb;
    vec4 texSample = texture(uTexo, TexCoord);
    if (texSample.a < 0.5) discard;
    fragColor = vec4(lit, 1.0) * texSample * fadeTo;

    vec2 screenUV = gl_FragCoord.xy / vec2(1280.0, 720.0);
    fragColor = drunkEffect(fragColor, screenUV, uTime, uDrunkenness);

    float fogDist  = length(vWorldPos - uCamPos);
    float fogFactor = clamp((fogDist - 25.0) / 175.0, 0.0, 1.0);
    vec3 fogColor = uAmbient * uLightColor.rgb;
    fragColor.rgb = mix(fragColor.rgb, fogColor, fogFactor);
}
