#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec2 uv;
} vs_out;

void main() {
    vec4 wp = model * vec4(aPos, 1.0);
    vs_out.worldPos = wp.xyz / wp.w;
    vs_out.normal = mat3(transpose(inverse(model))) * aNormal;
    vs_out.uv = aTex;
    gl_Position = projection * view * wp;
}