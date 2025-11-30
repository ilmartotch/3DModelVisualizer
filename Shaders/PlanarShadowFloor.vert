#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out VS_OUT {
    vec3 worldPos;
} vs_out;

void main() {
    vec4 worldPosition = model * vec4(aPos, 1.0);
    vs_out.worldPos = worldPosition.xyz;
    
    gl_Position = projection * view * worldPosition;
}