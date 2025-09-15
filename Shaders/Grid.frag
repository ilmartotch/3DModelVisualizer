#version 330 core
out vec4 FragColor;

// Dati interpolati dal vertex shader
in vec3 worldPos_var;

// Uniforms
uniform vec3 cameraPos;

// Colori
const vec3 gridColor = vec3(0.5, 0.5, 0.5);
const vec3 xAxisColor = vec3(1.0, 0.2, 0.2); // Rosso per asse X (lungo Z)
const vec3 zAxisColor = vec3(0.2, 0.2, 1.0); // Blu per asse Z (lungo X)

// Parametri
const float fadeStart = 50.0;
const float fadeEnd = 100.0;
const float gridLineWidth = 0.02;

// Funzione per disegnare le linee della griglia
float drawGrid(vec2 coord) {
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
    float line = min(grid.x, grid.y);
    return 1.0 - min(line, 1.0);
}

void main() {
    // Direzione del raggio dal punto di vista della camera
    vec3 rayDir = normalize(worldPos_var - cameraPos);

    // Normale del piano della griglia (punta verso l'alto)
    vec3 planeNormal = vec3(0.0, 1.0, 0.0);

    // Calcola il prodotto scalare tra la normale del piano e la direzione del raggio.
    // Se il valore è vicino a zero, la camera guarda parallelamente al piano.
    // Se il prodotto scalare ha lo stesso segno della coordinata Y della camera,
    // significa che stiamo guardando "lontano" dal piano, quindi non dobbiamo disegnarlo.

    float plane_dot_ray = dot(planeNormal, rayDir);
    if (abs(plane_dot_ray) < 0.001 || sign(plane_dot_ray) == sign(cameraPos.y)) {
        discard;
    }

    // Calcola l'intersezione del raggio di vista con il piano y=0
    float t = -cameraPos.y / rayDir.y;
    vec3 worldPos = cameraPos + rayDir * t;

    // Calcola la distanza dalla camera per la dissolvenza
    float dist = length(worldPos - cameraPos);
    float fade = 1.0 - smoothstep(fadeStart, fadeEnd, dist);
    if (fade < 0.01) {
        discard;
    }

    // Disegna griglia principale e secondaria
    float gridMajor = drawGrid(worldPos.xz);
    float gridMinor = drawGrid(worldPos.xz * 10.0) * 0.3;
    float gridLines = gridMajor + gridMinor * (1.0 - gridMajor);

    // Determina il colore
    vec3 finalColor = gridColor;
    if (abs(worldPos.z) < gridLineWidth) finalColor = xAxisColor;
    if (abs(worldPos.x) < gridLineWidth) finalColor = zAxisColor;

    // Calcola l'alpha finale
    float alpha = gridLines * fade;
    if (abs(worldPos.z) < gridLineWidth || abs(worldPos.x) < gridLineWidth) {
        alpha = min(1.0, alpha + 0.5);
    }

    FragColor = vec4(finalColor, alpha);
}