#version 330 core

// Input vertex data
layout(location = 0) in vec3 position;
layout(location = 3) in mat4 instanceMatrix;

// Uniform matrices
uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;  // Usato solo per rendering non istanziato

// Distingui tra rendering istanziato e non istanziato
uniform bool useInstancing;

void main() {
    // Se useInstancing è vero, usa la matrice dell'istanza
    // Altrimenti usa la matrice modello uniforme
    mat4 modelMatrix = useInstancing ? instanceMatrix : model;
    
    // Calcola la posizione del vertice nello spazio clip
    gl_Position = projection * view * modelMatrix * vec4(position, 1.0);
}