#version 330 core

// Output per il buffer di picking
layout(location = 0) out vec4 outColor;   // Regular color (non usato per il picking)
layout(location = 1) out vec4 outIdColor; // ID color per il picking

// ID color dell'oggetto
uniform vec3 idColor;

void main() {
    // Scrivi il colore normale (per il debug)
    outColor = vec4(0.5, 0.5, 0.5, 1.0);
    
    // Scrivi l'ID color nel secondo render target
    outIdColor = vec4(idColor, 1.0);
}