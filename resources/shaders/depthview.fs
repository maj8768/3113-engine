#version 330

// DEBUG: visualise a camera-view depth texture as LINEAR distance so it's legible.
// Raw window depth sits near 1.0 with a 1e18 far plane (looks flat white); we
// linearise it to world distance and normalise over a near range so geometry reads
// clearly: near = black, far = white.

in vec2 fragTexCoord;
uniform sampler2D texture0; // the texture passed to DrawTexture*
out vec4 finalColor;

void main() {
    float d = texture(texture0, fragTexCoord).r;
    // near 0.1 / far 1e18  ->  projA = -1, projB = -0.2 (see projMtx44)
    float ndc  = d * 2.0 - 1.0;
    float dist = -0.2 / (ndc - 1.0);      // world distance from camera
    float v    = clamp(dist / 150.0, 0.0, 1.0); // 0..150 units -> black..white
    finalColor = vec4(v, v, v, 1.0);
}
