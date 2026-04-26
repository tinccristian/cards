#version 330

// Heartbeat post-process effect.
// Samples the scene texture and applies:
//   - two-beat (lub-dub) zoom pulse, repeated once at lower intensity
//   - red vignette on the margins (edges), not the center
//   - subtle darkening at the edges on each beat

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float elapsed;   // seconds since effect started
uniform float fade;      // 1.0 at start → 0.0 at end

// Gaussian bump centered at `center` with sigma 0.10 s.
float beat(float t, float center) {
    float d = (t - center) / 0.10;
    return exp(-d * d);
}

void main() {
    // First lub-dub: full strength.
    float lub1 = beat(elapsed, 0.25);
    float dub1 = beat(elapsed, 0.55) * 0.70;

    // Second lub-dub: weaker echo beat.
    float lub2 = beat(elapsed, 1.50) * 0.40;
    float dub2 = beat(elapsed, 1.80) * 0.28;

    float pulse = (lub1 + dub1 + lub2 + dub2) * fade;

    // Slight zoom-in at the peak of each beat.
    float scale  = 1.0 + 0.025 * pulse;
    vec2  center = vec2(0.5, 0.5);
    vec2  uv     = center + (fragTexCoord - center) / scale;

    vec4 scene = texture(texture0, uv);

    // Edge factor: 0 at center, 1 at margins.
    float dist       = distance(fragTexCoord, center);
    float edgeFactor = smoothstep(0.25, 0.68, dist);

    // Red tint concentrated on the margins.
    scene.rgb += vec3(0.35, 0.0, 0.0) * edgeFactor * pulse;

    // Subtle darkening vignette at the edges.
    scene.rgb *= 1.0 - edgeFactor * 0.45 * pulse;
    scene.rgb  = clamp(scene.rgb, 0.0, 1.0);

    finalColor = scene;
}
