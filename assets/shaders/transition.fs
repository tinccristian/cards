#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D texture0;
uniform float progress;
uniform float hex_size;
uniform float randomness;
uniform float edge_softness;
uniform float transition_speed;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

vec2 hextile(inout vec2 p) {
    const vec2 spacing = vec2(1.0, 1.7320508);
    const vec2 halfSpacing = spacing * 0.5;

    vec2 a = mod(p, spacing) - halfSpacing;
    vec2 b = mod(p - halfSpacing, spacing) - halfSpacing;

    vec2 cell = dot(a, a) < dot(b, b) ? a : b;
    vec2 id = (cell - p + halfSpacing) / spacing;

    p = cell;
    return floor(id);
}

float hexDistance(vec2 p, float radius) {
    p = abs(p);
    return max(p.x * 0.866025 + p.y * 0.5, p.y) - radius;
}

void main() {
    vec2 uv = fragTexCoord;
    vec4 fromColor = texture(texture0, vec2(uv.x * 0.5, uv.y));
    vec4 toColor = texture(texture0, vec2(0.5 + uv.x * 0.5, uv.y));

    if (progress <= 0.0) {
        finalColor = fromColor;
        return;
    }

    if (progress >= 1.0) {
        finalColor = toColor;
        return;
    }

    vec2 screenSize = vec2(textureSize(texture0, 0));
    screenSize.x *= 0.5;
    vec2 p = uv - 0.5;
    float aspect = screenSize.x / max(screenSize.y, 1.0);
    p.x *= aspect * 0.9;

    vec2 hexPoint = p / max(hex_size, 0.0001);
    vec2 hexId = hextile(hexPoint);

    float delay = (uv.x + uv.y) * 0.5;
    float revealBand = smoothstep(
        delay - transition_speed,
        delay + transition_speed,
        progress * (1.0 + transition_speed)
    );
    float jitter = hash(hexId);
    float tileProgress = clamp(revealBand - jitter * randomness, 0.0, 1.0);
    float distanceToHexEdge = hexDistance(hexPoint, tileProgress * 0.9);
    float mask = smoothstep(edge_softness, -edge_softness, distanceToHexEdge);

    finalColor = mix(fromColor, toColor, mask);
}
