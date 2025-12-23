#version 450 core

in VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec2 uv;
} fs_in;

out vec4 FragColor;

struct DirLight {
    int  enabled;
    vec3 direction;
    vec3 color;
};

uniform DirLight uDirLight;

uniform sampler2D textureSampler;
uniform sampler2D uShadowMap;

uniform mat4 lightSpaceMatrix;
uniform int  uUseShadowMap;
uniform int  uShadowMapSize;
uniform float uFloorHeight;

uniform int useTexture;
uniform int useOverrideColor;
uniform vec4 overrideColor;

uniform int uLightIcon;
uniform int uLightGradientEnabled;
uniform vec3 uLightGradientStart;
uniform vec3 uLightGradientEnd;
uniform vec3 uLightEmitColor;

uniform int uUnlit;

uniform vec3 viewPos;

// PCF con kernel 3x3
float computeShadow(vec3 worldPos, vec3 normal) {
    if (uUseShadowMap == 0) return 0.0;

    vec4 lsPos = lightSpaceMatrix * vec4(worldPos, 1.0);
    vec3 projCoords = lsPos.xyz / lsPos.w;

    // NDC out-of-range => not in the shadow
    if (projCoords.x < -1.0 || projCoords.x > 1.0 ||
        projCoords.y < -1.0 || projCoords.y > 1.0 ||
        projCoords.z <  0.0 || projCoords.z > 1.0) {
        return 0.0;
    }

    // the projCoords z component is in [-1, 1] range
    vec3 uv = projCoords * 0.5 + 0.5;

    float angle = max(dot(normalize(normal), normalize(-uDirLight.direction)), 0.0);
    float bias = max(0.005 * (1.0 - angle), 0.0005);

    float shadow = 0.0;
    float texel = 1.0 / float(uShadowMapSize);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(x, y) * texel;
            float depth = texture(uShadowMap, uv.rg + offset).r;
            shadow += (depth + bias) < uv.z ? 1.0f : 0.0f;
        }
    }
    shadow /= 9.0;
    return shadow;
}

vec3 baseAlbedo() {
    if (useOverrideColor == 1) {
        return overrideColor.rgb;
    } else if (useTexture == 1) {
        return texture(textureSampler, fs_in.uv).rgb;
    } else {
        return vec3(0.55);
    }
}

void main() {
    if (uLightIcon == 1) {
        vec3 c = uLightEmitColor;
        if (uLightGradientEnabled == 1) {
            float t = clamp(normalize(fs_in.normal).y * 0.5 + 0.5, 0.0, 1.0);
            c = mix(uLightGradientEnd, uLightGradientStart, t);
        }
        FragColor = vec4(c, 1.0);
        return;
    }

    vec3 N = normalize(fs_in.normal);
    vec3 V = normalize(viewPos - fs_in.worldPos);
    vec3 L = normalize(-uDirLight.direction);

    vec3 albedo = baseAlbedo();

    if (uUnlit == 1) {
        FragColor = vec4(albedo, 1.0);
        return;
    }

    float diff = max(dot(N, L), 0.0);

    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 64.0);

    float shadow = computeShadow(fs_in.worldPos, N);
    vec3 ambient = albedo * 0.15;
    ambient = vec3(0.0f); 
    // Disable ambient

    vec3 lightColor = uDirLight.color;
    vec3 lit = (1.0 - shadow) * (albedo * diff + lightColor * spec * 0.3) + ambient;

    FragColor = vec4(lit, 1.0);
}