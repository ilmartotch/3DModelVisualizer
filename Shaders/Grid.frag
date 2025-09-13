#version 330 core
out vec4 FragColor;

// Posizione del frammento nel mondo (ricevuta dal vertex shader)
in vec3 worldPos;

// Posizione della camera per calcolare la dissolvenza
uniform vec3 cameraPos;

// Colore della griglia, dell'asse X e dell'asse Z
uniform vec3 gridColor = vec3(0.5, 0.5, 0.5);
const vec3 xAxisColor = vec3(1.0, 0.2, 0.2); // Rosso
const vec3 zAxisColor = vec3(0.2, 0.2, 1.0); // Blu

// Parametri per la griglia
const float gridSize = 1.0;      // Spaziatura delle linee principali
const float fadeDistance = 50.0; // Distanza a cui la griglia inizia a svanire

void main() {
    // Calcola la distanza del frammento dalla camera sul piano XZ
    float distance = length(worldPos.xz - cameraPos.xz);

    // Calcola l'opacità (alpha) basandoti sulla distanza per creare un effetto di dissolvenza
    float alpha = 1.0 - smoothstep(fadeDistance - 10.0, fadeDistance, distance);
    if (alpha < 0.0) discard; // Non disegnare frammenti completamente trasparenti

    // Algoritmo per disegnare le linee della griglia
    // fwidth calcola la differenza tra i valori adiacenti, aiutando a mantenere le linee sottili e definite
    vec2 grid = abs(fract(worldPos.xz / gridSize - 0.5) - 0.5) / fwidth(worldPos.xz / gridSize);
    float line = min(grid.x, grid.y);

    // Se il frammento è molto vicino a una linea (valore basso), lo coloriamo
    if (line < 1.0) {
        // Inizia con il colore di base della griglia
        vec3 finalColor = gridColor;

        // Controlla se siamo vicini all'asse Z (x=0) e coloralo di blu
        // Usiamo fwidth per mantenere la linea dell'asse spessa quanto le altre linee della griglia
        if (abs(worldPos.x) < fwidth(worldPos.x) * 1.5) {
            finalColor = zAxisColor;
        }
        // Controlla se siamo vicini all'asse X (z=0) e coloralo di rosso
        if (abs(worldPos.z) < fwidth(worldPos.z) * 1.5) {
            finalColor = xAxisColor;
        }

        float lineIntensity = 1.0 - smoothstep(0.95, 1.0, line);
        FragColor = vec4(finalColor, lineIntensity * alpha);
    } else {
        // Se non siamo su una linea, scartiamo il frammento per rendere la griglia trasparente
        discard;
    }
}