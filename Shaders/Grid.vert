#version 330 core
layout (location = 0) in vec3 aPos;

// Matrici per la trasformazione
uniform mat4 projection;
uniform mat4 view;

// La posizione del vertice nel mondo la calcoliamo qui
out vec3 worldPos;

void main() {
    // Calcoliamo la posizione del vertice nel mondo.
    // Ignoriamo la matrice 'model' perché il nostro quad è già in coordinate del mondo.
    worldPos = aPos;
    
    // Trasformiamo la posizione per la visualizzazione
    gl_Position = projection * view * vec4(aPos, 1.0);
}