#version 330

layout(location = 0) in vec3 instanceCenter;
layout(location = 1) in vec3 instanceRight;
layout(location = 2) in vec3 instanceUp;
layout(location = 3) in vec4 instanceBottomColor;
layout(location = 4) in vec4 instanceTopColor;
layout(location = 5) in vec4 instanceUV; // offset.xy, scale.zw

uniform mat4 uVP;

out vec2 fragTexCoord;
out vec4 fragColor;

void main() {
    const vec2 corners[6] = vec2[6](
        vec2(-1.0, 1.0), vec2(-1.0, -1.0), vec2(1.0, -1.0),
        vec2(-1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0)
    );
    vec2 corner = corners[gl_VertexID];
    vec3 position = instanceCenter + instanceRight * corner.x + instanceUp * corner.y;
    vec2 uv = vec2(corner.x * 0.5 + 0.5, 0.5 - corner.y * 0.5);
    fragTexCoord = instanceUV.xy + uv * instanceUV.zw;
    fragColor = mix(instanceBottomColor, instanceTopColor, corner.y * 0.5 + 0.5);
    gl_Position = uVP * vec4(position, 1.0);
}
