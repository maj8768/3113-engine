#version 410

in vec4 vColor;
in vec3 vNormal;
in vec2 TexCoord;
in vec3 fragPos;

uniform vec3  uLightPos;
uniform vec4  uLightColor;
uniform float uAmbient;
uniform sampler2D uTexo;
uniform float fadeTo;

#define MAX_BOXES 8
uniform vec3 uBoxMins[MAX_BOXES];
uniform vec3 uBoxMaxs[MAX_BOXES];
uniform int  uBoxCount;
uniform int  uShadowsEnabled;

out vec4 fragColor;


// https://tavianator.com/2011/ray_box.html
bool rayAABB(vec3 origin, vec3 dir, vec3 bmin, vec3 bmax, out float tmin) {
    vec3 invDir = 1.0 / dir;

    vec3 t0 = (bmin - origin) * invDir;
    vec3 t1 = (bmax - origin) * invDir;

    vec3 tNear = min(t0, t1);
    vec3 tFar = max(t0, t1);
    float tmax = min(min(tFar.x, tFar.y), tFar.z);

    tmin = max(max(tNear.x, tNear.y), tNear.z);
    return tmax >= tmin && tmax > 0.0;
}

// https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
// https://www.pbr-book.org/3ed-2018/Light_Sources/Light_Interface?utm_source=chatgpt.com
float ShadowCalculation(vec3 worldPos) {
    vec3  origin = worldPos + normalize(vNormal) * 0.05;
    vec3  toLight = uLightPos - origin;
    float distToLight = length(toLight);
    vec3  dir = toLight / distToLight;

    for (int i = 0; i < uBoxCount; i++) {
        vec3 eps3 = vec3(0.05);
        if (all(greaterThanEqual(worldPos, uBoxMins[i] - eps3)) && all(lessThanEqual(worldPos, uBoxMaxs[i] + eps3))) {
            continue;
        }

        float tmin;
        if (rayAABB(origin, dir, uBoxMins[i], uBoxMaxs[i], tmin)) {
            if (tmin > 0.0 && tmin < distToLight) {
                return 1.0;
            }
        }
    }
    return 0.0;
}

void main() {
    vec3  n = normalize(vNormal);
    vec3  lightVec = uLightPos - fragPos;
    float dist = length(lightVec);
    vec3  toLight = lightVec / dist;

    float diffuse = max(dot(n, toLight), 0.0);
    float attenuation = 1.0 / (1.0 + 0.01 * dist + 0.002 * dist * dist);
    float shadow = uShadowsEnabled != 0 ? ShadowCalculation(fragPos) : 0.0;

    vec3 lit = (uAmbient + (1.0 - shadow) * diffuse * attenuation) * uLightColor.rgb;
    fragColor = vec4(lit, 1.0) * texture(uTexo, TexCoord) * fadeTo;
}
