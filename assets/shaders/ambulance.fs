#version 330

// Ambulance lights post-process effect.
// Samples the scene texture and additively blends two Gaussian light beams
// (red + blue) sweeping in opposite directions.

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float elapsed;   // seconds since effect started
uniform float fade;      // 1.0 at start → 0.0 at end

void main() {
    vec4 scene = texture(texture0, fragTexCoord);

    float speed = 2.5;
    float phase = sin(elapsed * speed);
    float cR    = phase * 0.5 + 0.5;
    float cB    = -phase * 0.5 + 0.5;

    float u     = fragTexCoord.x;
    float sigma = 0.28;
    float dR    = (u - cR) / sigma;
    float dB    = (u - cB) / sigma;
    float iR    = exp(-dR * dR);
    float iB    = exp(-dB * dB);

    float total = iR + iB;
    if (total > 0.01) {
        float blendT = iB / total;
        vec3  light  = mix(vec3(1.0, 0.0, 0.0), vec3(0.1, 0.1, 1.0), blendT);
        float alpha  = 0.22 * min(total, 1.0) * fade;
        scene.rgb   += light * alpha;
        scene.rgb    = clamp(scene.rgb, 0.0, 1.0);
    }

    finalColor = scene;
}
