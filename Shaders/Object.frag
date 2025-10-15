#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D textureSampler;
uniform vec3 viewPos;
uniform float time;

// Uniforms per il colore di override
uniform bool useOverrideColor;
uniform vec4 overrideColor;

void main() {
    
    vec3 lightPos = vec3(2.0, 5.0, 2.0);
    vec3 lightColor = vec3(1.0, 1.0,1.0);
    
    // Ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec4 texColor = texture(textureSampler, TexCoord);
    
    // Evita che gli oggetti neri diventino invisibili
    if (length(texColor.rgb) < 0.1) {
        texColor.rgb = vec3(0.2, 0.2, 0.2);
    }

    vec3 litColor = (ambient + diffuse + specular) * texColor.rgb;

    if(useOverrideColor) {
        litColor *= overrideColor.rgb;

        float alpha = texColor.a * overrideColor.a;
        FragColor = vec4(litColor, alpha);
    }
    else {
        FragColor = vec4(litColor, texColor.a);
    }
}