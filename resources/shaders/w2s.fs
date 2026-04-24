#version 410

in vec4 vColor;
in vec3 vNormal;
in vec2 TexCoord;
in vec4 fragPosLightSpace;
in vec3 vWorldPos;

uniform vec3 uLightDir;
uniform vec4 uLightColor;
uniform float uAmbient;
uniform sampler2D uTexo;
uniform sampler2D uShadowMap;
uniform sampler2D uShadowMapFar;
uniform float fadeTo;
uniform float uTime;
uniform float uDrunkenness;
uniform int uShadowsEnabled;
uniform vec3 uCamPos;
uniform mat4 uLightSpaceMatrixFar;
uniform float uCascadeSplit;

#define MAX_POINT_LIGHTS 8
uniform vec3 uPointPos[MAX_POINT_LIGHTS];
uniform vec3 uPointColor[MAX_POINT_LIGHTS];
uniform float uPointRadius[MAX_POINT_LIGHTS];
uniform int uPointCount;
uniform float uDrankUrgency;
uniform vec2  uResolution;

out vec4 fragColor;

float sampleShadowBilinear(sampler2D sm, vec2 uv, float refDepth, vec2 ts) {
    vec2 f = fract(uv / ts);
    vec2 base = floor(uv / ts) * ts;
    float s00 = float(refDepth > texture(sm, base).r);
    float s10 = float(refDepth > texture(sm, base + vec2(ts.x, 0.0)).r);
    float s01 = float(refDepth > texture(sm, base + vec2(0.0, ts.y)).r);
    float s11 = float(refDepth > texture(sm, base + ts).r);
    return mix(mix(s00, s10, f.x), mix(s01, s11, f.x), f.y);
}

float ShadowPCF(sampler2D sm, vec4 lightSpacePos, float ndotl, float spread) {
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z < 0.0 || projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float bias = max(0.003 * (1.0 - ndotl), 0.001);
    float refDepth = projCoords.z - bias;
    vec2 texelSize = 1.0 / vec2(textureSize(sm, 0));

    float sum = 0.0, totalWeight = 0.0;
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            float w = exp(-float(x*x + y*y) * 0.2);
            vec2 uv = projCoords.xy + vec2(float(x), float(y)) * texelSize * spread;
            sum += w * sampleShadowBilinear(sm, uv, refDepth, texelSize);
            totalWeight += w;
        }
    }
    return sum / totalWeight;
}

float ShadowCalculation(float ndotl) {
    float fragDist = length(vWorldPos - uCamPos);
    float blend = smoothstep(uCascadeSplit * 0.8, uCascadeSplit, fragDist);

    float shadowNear = ShadowPCF(uShadowMap, fragPosLightSpace, ndotl, 4.0);
    vec4 farPos = uLightSpaceMatrixFar * vec4(vWorldPos, 1.0);
    float shadowFar = ShadowPCF(uShadowMapFar, farPos, ndotl, 3.0);

    return mix(shadowNear, shadowFar, blend);
}

vec4 drunkEffect(vec4 color, vec2 screenUV, float time, float drunkenness) {
    drunkenness = clamp(drunkenness, 0.0, 1.0);
    if (drunkenness <= 0.0) return color;

    vec2 uv = screenUV;
    vec2 center = vec2(0.5, 0.5);
    vec2 toEdge = uv - center;
    float dist = length(toEdge);

    float squintT = clamp((drunkenness - 0.5) / 0.5, 0.0, 1.0);

    float aberrStr = drunkenness * 0.018;
    vec2 aberrDir = normalize(toEdge + vec2(1e-5)) * aberrStr;
    float rBoost = dot(toEdge, aberrDir) * 6.0;
    float bBoost = dot(toEdge, -aberrDir) * 6.0;
    color.r = clamp(color.r + rBoost * drunkenness, 0.0, 1.0);
    color.b = clamp(color.b + bBoost * drunkenness, 0.0, 1.0);

    float sqBlur = squintT * 0.12;
    vec2 sqDir = normalize(toEdge + vec2(1e-5)) * sqBlur;
    float sqRBoost = dot(toEdge, sqDir) * 8.0;
    float sqBBoost = dot(toEdge, -sqDir) * 8.0;
    float sqLum = color.r * 0.299 + color.g * 0.587 + color.b * 0.114;
    vec3 sqGrad = vec3(dFdx(sqLum), dFdy(sqLum), 0.0);
    float smear = (sqGrad.x + sqGrad.y) * squintT * 6.0;
    color.r = clamp(color.r + sqRBoost * squintT + smear, 0.0, 1.0);
    color.g = clamp(color.g + smear * 0.5, 0.0, 1.0);
    color.b = clamp(color.b + sqBBoost * squintT + smear, 0.0, 1.0);

    float vibAmp = drunkenness * 0.03;
    float vibRate = 18.0;
    float rowShift = sin(time * vibRate + uv.y * 25.0) * vibAmp;
    float vertShift = sin(time * vibRate * 0.7 + 1.3) * vibAmp * 0.5;
    float hGrad = dFdx(color.r + color.g + color.b);
    float vGrad = dFdy(color.r + color.g + color.b);
    color.rgb += rowShift * hGrad * 40.0;
    color.rgb += vertShift * vGrad * 40.0;
    color.rgb = clamp(color.rgb, 0.0, 1.0);

    float rayLength = drunkenness * 0.45;
    float edgeFade = smoothstep(0.5 - rayLength, 0.5, dist);
    float angle = atan(toEdge.y, toEdge.x);
    float raySpokes = abs(sin(angle * 5.0 + time * 0.7));
    float pulse = sin(time * 2.5) * 0.5 + 0.5;
    float ray = edgeFade * raySpokes * pulse * drunkenness * 0.5;
    color.rgb = clamp(color.rgb + ray, 0.0, 1.5);

    return color;
}

vec4 drankUrgencyEffect(vec4 color, vec2 screenUV, float urgency) {
    if (urgency <= 0.0) return color;
    if (urgency >= 1.0) return vec4(0.0, 0.0, 0.0, color.a);

    vec2 centered = screenUV - vec2(0.5);
    centered.x *= uResolution.x / uResolution.y;
    float dist = length(centered);

    float outerEdge = mix(1.2, 0.02, urgency);
    float softness = max(0.01, outerEdge * 0.55);
    float vignette = smoothstep(outerEdge, outerEdge - softness, dist);

    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    vec3 desat = mix(color.rgb, vec3(gray), urgency);

    return vec4(desat * vignette, color.a);
}

void main() {
    vec3 n = normalize(vNormal);
    vec3 toLight = normalize(-uLightDir);

    float diffuse = max(dot(n, toLight), 0.0);
    float ndotl = diffuse;
    float shadow = (uShadowsEnabled != 0) ? ShadowCalculation(ndotl) : 0.0;
    vec3 lit = (uAmbient + (1.0 - shadow) * diffuse) * uLightColor.rgb;

    for (int i = 0; i < uPointCount; i++) {
        vec3 toPoint = uPointPos[i] - vWorldPos;
        float dist = length(toPoint);
        float atten = max(0.0, 1.0 - dist / uPointRadius[i]);
        atten *= atten;
        lit += max(dot(n, normalize(toPoint)), 0.0) * atten * uPointColor[i];
    }

    vec4 texSample = texture(uTexo, TexCoord);
    if (texSample.a < 0.5) discard;

    fragColor = vec4(lit, 1.0) * texSample * fadeTo;

    vec2 screenUV = gl_FragCoord.xy / uResolution;
    fragColor = drunkEffect(fragColor, screenUV, uTime, uDrunkenness);

    float fogDist = length(vWorldPos - uCamPos);
    float fogFactor = clamp((fogDist - 5.0) / 95.0, 0.0, 1.0);
    vec3 fogColor = uAmbient * uLightColor.rgb;
    fragColor.rgb = mix(fragColor.rgb, fogColor, fogFactor);

    fragColor = drankUrgencyEffect(fragColor, screenUV, uDrankUrgency);
}