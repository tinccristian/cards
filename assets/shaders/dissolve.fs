#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

// 0.0 = fully visible, 1.0 = fully dissolved.
uniform float sensitivity;
// Full texture dimensions in pixels (used for pixel-grid snapping).
uniform vec2  texSize;
// Elapsed time in seconds for animated wind turbulence.
uniform float time;

out vec4 finalColor;

float random(vec2 uv) {
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 438.5453);
}

void main() {
    // Snap UV to pixel boundaries for a stable per-pixel dissolve pattern.
    vec2 UVr = vec2(
        floor(fragTexCoord.x * texSize.x) / texSize.x,
        floor(fragTexCoord.y * texSize.y) / texSize.y
    );

    // Per-pixel dissolve threshold — constant across animation frames.
    float threshold = random(UVr);

    // Wind displacement: pixels near the dissolve boundary drift before vanishing.
    // edgeFactor: 1.0 far from the dissolve edge, 0.0 right at it.
    float edgeFactor = clamp((threshold - sensitivity) / 0.25, 0.0, 1.0);
    float windForce  = 1.0 - edgeFactor;

    // Rightward wind with animated vertical turbulence.
    float dispX =  windForce * (4.0 + sin(time * 3.0 + UVr.y * 20.0)) / texSize.x;
    float dispY =  windForce * sin(time * 2.5 + UVr.x * 15.0 + UVr.y * 5.0) * 2.0 / texSize.y;

    vec2 displacedUV = clamp(fragTexCoord + vec2(dispX, dispY), 0.0, 1.0);
    vec4 pixelColor  = texture(texture0, displacedUV) * colDiffuse * fragColor;

    // Binary dissolve: pixels with threshold < sensitivity vanish.
    float visible = step(sensitivity, threshold);

    finalColor = vec4(pixelColor.rgb, pixelColor.a * visible);
}
