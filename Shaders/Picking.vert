#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

layout (location = 3) in vec4 aInstanceMatrix0;
layout (location = 4) in vec4 aInstanceMatrix1;
layout (location = 5) in vec4 aInstanceMatrix2;
layout (location = 6) in vec4 aInstanceMatrix3;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform bool useInstancing = false;

void main()
{
    mat4 finalModel;
    
    if (useInstancing) {
        finalModel = mat4(
            aInstanceMatrix0,
            aInstanceMatrix1,
            aInstanceMatrix2,
            aInstanceMatrix3
        );
    } else {
        finalModel = model;
    }
    
    gl_Position = projection * view * finalModel * vec4(aPos, 1.0);
}