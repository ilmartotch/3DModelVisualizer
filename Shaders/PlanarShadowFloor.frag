#version 450 core

in VS_OUT {
    vec3 worldPos;
} fs_in;

out vec4 FragColor;

uniform vec3 uBackgroundColor;
uniform sampler2D uShadowMap;
uniform mat4 lightSpaceMatrix;
uniform int  uUseShadowMap;
uniform int  uShadowMapSize;
uniform float uFloorHeight;

uniform vec3 uLightDir;
uniform float uShadowStrength;

float randHash2(vec2 p) {
    p = fract(p * vec2(91.42, 73.12));
    p += dot(p, p + 24.57);
    return fract(p.x * p.y);
}

float computeShadowPlanar(vec3 worldPos) {
    if (uUseShadowMap == 0) return 0.0;

    vec4 lsPos = lightSpaceMatrix * vec4(worldPos, 1.0);
    vec3 projCoords = lsPos.xyz / lsPos.w;

    if (projCoords.x < -1.0 || projCoords.x > 1.0 ||
        projCoords.y < -1.0 || projCoords.y > 1.0 ||
        projCoords.z <  0.0 || projCoords.z > 1.0) {
        return 0.0;
    }

    vec3 uv = projCoords * 0.5 + 0.5;

    vec3 N = vec3(0, 1, 0);
    float ndotl = max(dot(N, normalize(uLightDir)), 0.0);

    float bias = max(0.005 * (1.0 - ndotl), 0.0005);

    float shadow = 0.0;
    float texel = 1.0 / float(uShadowMapSize);
    float radius = 2.0;

    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 offset = vec2(x, y) * texel * radius;

            float depth = texture(uShadowMap, uv.xy + offset).r;
            shadow += (depth + bias) < uv.z ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;
    shadow = pow(clamp(shadow, 0.0, 1.0), 1.1);

    return shadow; // 0 = no shadow, 1 = full shadow
}

void main() {
    float shadow = computeShadowPlanar(fs_in.worldPos);
    float shade = clamp(1.0 - uShadowStrength * shadow, 0.0, 1.0);
    vec3 color = uBackgroundColor * shade;
    FragColor = vec4(color, 1.0);
}