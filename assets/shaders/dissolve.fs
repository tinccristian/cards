#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float sensitivity; // 0.0 visible -> 1.0 gone
uniform vec2 texSize;
uniform float time;

out vec4 finalColor;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void main() {
    vec2 snappedUV = vec2(
        floor(fragTexCoord.x * texSize.x) / texSize.x,
        floor(fragTexCoord.y * texSize.y) / texSize.y
    );

    vec4 source = texture(texture0, snappedUV) * colDiffuse * fragColor;
    if (source.a <= 0.0) {
        discard;
    }

    float grain = noise(snappedUV * texSize * 0.22);
    float verticalBias = pow(1.0 - snappedUV.y, 1.6);
    float threshold = grain * 0.55 + verticalBias * 0.45;

    float edgeBand = 0.08;
    float edge = smoothstep(sensitivity - edgeBand, sensitivity + edgeBand, threshold);
    float dissolveMask = step(sensitivity, threshold);

    float ashLift = (1.0 - dissolveMask) * (0.012 + 0.008 * sin(time * 7.0 + snappedUV.x * 18.0));
    float ashSway = (1.0 - dissolveMask) * 0.006 * sin(time * 5.0 + snappedUV.y * 22.0);
    vec2 displacedUV = clamp(snappedUV + vec2(ashSway, -ashLift), 0.0, 1.0);

    vec4 displaced = texture(texture0, displacedUV) * colDiffuse * fragColor;
    vec3 ember = mix(vec3(0.78, 0.10, 0.05), vec3(1.0, 0.74, 0.28), 1.0 - edge);
    vec3 color = mix(ember, displaced.rgb, dissolveMask);

    float alpha = displaced.a * mix(0.0, 1.0, dissolveMask);
    float emberAlpha = displaced.a * (1.0 - dissolveMask) * (1.0 - edge) * 0.9;

    finalColor = vec4(mix(color, ember, emberAlpha), max(alpha, emberAlpha));
}
