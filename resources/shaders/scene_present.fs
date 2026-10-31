#version 330

uniform sampler2D uScene;
out vec4 finalColor;

void main() {
    // Source and destination have the same physical framebuffer size and orientation.
    finalColor = texelFetch(uScene, ivec2(gl_FragCoord.xy), 0);
}
