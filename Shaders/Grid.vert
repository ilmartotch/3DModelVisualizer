#version 450 core

layout (location = 0) in vec3 aPos;

// Matrici inverse passate come uniform
uniform mat4 invView;
uniform mat4 invProjection;

// Output al fragment shader
out vec3 worldPos_var;

void main() {

    vec4 world = invView * invProjection * vec4(aPos.xy, 0.999, 1.0);
    worldPos_var = world.xyz / world.w;

    gl_Position = vec4(aPos.xy, 0.999, 1.0);
}