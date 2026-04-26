#version 330

// Ambulance lights overlay.
// Two Gaussian light beams (red + blue) sweep across the screen in opposite
// directions and blend together where they overlap.

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform float elapsed;      // seconds since effect started
uniform float fade;         // 1.0 at start, 0.0 at end
uniform vec2  resolution;   // screen size in pixels

void main() {
    // Screen UV (0,0) = bottom-left in OpenGL, but we only care about x.
    float u = gl_FragCoord.x / resolution.x;

    float speed = 2.5;

    // Red and blue beams at opposite phases of the same sine wave.
    float phase = sin(elapsed * speed);
    float cR    = phase * 0.5 + 0.5;
    float cB    = -phase * 0.5 + 0.5;

    // Gaussian beam width — 28% of screen width.
    float sigma = 0.28;
    float dR    = (u - cR) / sigma;
    float dB    = (u - cB) / sigma;
    float iR    = exp(-dR * dR);
    float iB    = exp(-dB * dB);

    float total = iR + iB;
    if (total < 0.01) discard;

    // Blend color proportionally: pure red when only red beam, pure blue when
    // only blue beam, magenta where they overlap.
    float blendT = iB / total;
    vec3  color  = mix(vec3(1.0, 0.0, 0.0), vec3(0.1, 0.1, 1.0), blendT);

    float alpha = 0.22 * min(total, 1.0) * fade;
    finalColor  = vec4(color, alpha);
}
