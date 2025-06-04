#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vertexColor;

uniform mat4 model; //matrice di trasformazione
uniform mat4 view; // matrice di vista
uniform mat4 projection; // matrice di proiezione

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0); // Usa model invece di transform
	vertexColor = aColor; // Passa il colore al framment shader
}