#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Starting index into the palette (0 = red side, 3 = blue side).
uniform int   start_frame;
// Current animation frame (advances each tick to cycle colours).
uniform int   current_frame;
// Blend between original colour (0.0) and palette colour (1.0).
uniform float mix_ratio;

out vec4 finalColor;

// Red → dark-red → black → blue → dark-blue → black
const vec3 palette[6] = vec3[6](
    vec3(1.0, 0.0, 0.0),
    vec3(0.5, 0.0, 0.0),
    vec3(0.0, 0.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.0, 0.5),
    vec3(0.0, 0.0, 0.0)
);

void main() {
    vec4 texColor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;

    // Rec. 709 luma — brighter pixels get a higher palette offset.
    float brightness = dot(texColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    int offset = 0;
    if      (brightness > 0.75) offset = 2;
    else if (brightness > 0.25) offset = 1;

    int   color_index = (start_frame + current_frame + offset) % 6;
    vec3  color       = palette[color_index];

    finalColor = vec4(mix(texColor.rgb, color, mix_ratio), texColor.a);
}
