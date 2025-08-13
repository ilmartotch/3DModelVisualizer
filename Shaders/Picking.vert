#version 330 core

// Attributi dei vertici
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

// Attributi delle matrici di istanza per il rendering istanziato (opzionali)
layout (location = 3) in vec4 aInstanceMatrix0;
layout (location = 4) in vec4 aInstanceMatrix1;
layout (location = 5) in vec4 aInstanceMatrix2;
layout (location = 6) in vec4 aInstanceMatrix3;

// Uniformi per le matrici di trasformazione
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Flag per indicare se stiamo usando il rendering istanziato
uniform bool useInstancing = false;

void main()
{
    mat4 finalModel;
    
    if (useInstancing) {
        // Costruisce la matrice modello dall'istanza
        finalModel = mat4(
            aInstanceMatrix0,
            aInstanceMatrix1,
            aInstanceMatrix2,
            aInstanceMatrix3
        );
    } else {
        // Usa la matrice modello tradizionale
        finalModel = model;
    }
    
    // Calcola la posizione finale del vertice
    gl_Position = projection * view * finalModel * vec4(aPos, 1.0);
}