#version 450 core

// Output per il buffer di picking
layout(location = 0) out vec4 outColor;   // Regular color (per debug/visualizzazione)
layout(location = 1) out vec4 outIdColor; // ID color per il picking

// ID color dell'oggetto passato come uniform
uniform vec3 idColor;

void main() {
    // Scrivi il colore normale nel primo render target (per debug se necessario)
    outColor = vec4(0.5, 0.5, 0.5, 1.0);
    
    // Scrivi l'ID color nel secondo render target per il picking
    outIdColor = vec4(idColor, 1.0);
}