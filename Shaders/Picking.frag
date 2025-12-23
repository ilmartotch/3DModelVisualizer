#version 450 core

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outIdColor;

uniform vec3 idColor;

void main() {
    outColor = vec4(0.5, 0.5, 0.5, 1.0);
    outIdColor = vec4(idColor, 1.0);
}