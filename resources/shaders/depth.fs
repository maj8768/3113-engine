#version 410

in vec2 TexCoord;
uniform sampler2D uTexo;

void main() {
    if (texture(uTexo, TexCoord).a < 0.5) discard;
}
