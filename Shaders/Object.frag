#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
// Rimuovi l'input VertexColor

out vec4 FragColor;

// Uniforms texture
uniform sampler2D textureSampler;
// Rimuovi uniform per arcobaleno
// uniform bool useRainbow;
// uniform float time;
// uniform float rainbowSpeed;

void main() {
    // Usa semplicemente la texture (o il colore grigio di default)
    vec4 color = texture(textureSampler, TexCoord);
    
    // Illuminazione semplice
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normalize(Normal), lightDir), 0.0);
    vec3 diffuse = diff * vec3(0.8);
    vec3 ambient = vec3(0.2);
    
    // Risultato finale
    FragColor = vec4((ambient + diffuse) * color.rgb, color.a);
}