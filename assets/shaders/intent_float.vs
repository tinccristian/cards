#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

out vec2 fragTexCoord;
out vec4 fragColor;

uniform mat4 mvp;
uniform float time;
uniform float amplitude;
uniform float speed;
uniform float phase;

void main() {
    vec3 position = vertexPosition;
    position.y += sin(time * speed + phase) * amplitude;

    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp * vec4(position, 1.0);
}
