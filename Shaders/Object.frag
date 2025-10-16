#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D textureSampler;
uniform vec3 viewPos;
uniform float time;

// Flag per controllare texture e colore
uniform int useTexture;
uniform int useOverrideColor;
uniform vec4 overrideColor;

void main()
{
    vec4 baseColor;
    
    // Determina il colore base
    if (useOverrideColor == 1) {
        // Usa il colore override SENZA texture
        baseColor = overrideColor;
    } else if (useTexture == 1) {
        // Usa la texture
        baseColor = texture(textureSampler, TexCoord);
        
        // Se la texture ha alpha molto basso, usa un colore di fallback
        if (baseColor.a < 0.01) {
            baseColor = vec4(0.8, 0.8, 0.8, 1.0);
        }
    } else {
        // Colore di default (grigio chiaro)
        baseColor = vec4(0.8, 0.8, 0.8, 1.0);
    }
    
    // Illuminazione semplice
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    
    vec3 ambient = 0.3 * baseColor.rgb;
    vec3 diffuse = diff * baseColor.rgb;
    
    vec3 result = ambient + diffuse;
    
    FragColor = vec4(result, baseColor.a);
}