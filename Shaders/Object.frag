#version 330 core

out vec4 FragColor;

in vec2 v_texCoords;
in vec3 v_normal;
in vec3 v_fragPos;

uniform sampler2D textureSampler;
uniform vec3 viewPos;

// Uniforms per il colore di override
uniform bool useOverrideColor;
uniform vec4 overrideColor;

void main() {
    // Se il colore di override è attivo, usalo e termina
    if (useOverrideColor) {
        FragColor = overrideColor;
        return;
    }

    // Altrimenti, procedi con la logica di illuminazione e texture standard
    vec3 lightPos = vec3(2.0, 5.0, 2.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    
    // Ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(v_normal);
    vec3 lightDir = normalize(lightPos - v_fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - v_fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec4 texColor = texture(textureSampler, v_texCoords);
    
    // Evita che gli oggetti neri diventino invisibili
    if (texColor.rgb == vec3(0.0, 0.0, 0.0)) {
        texColor.rgb = vec3(0.05, 0.05, 0.05);
    }

    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}