#version 450 core

layout(depth_less) out float gl_FragDepth;
out vec4 FragColor;

in vec3 worldPos_var;

uniform vec3 cameraPos;
uniform int  uInfinite;
uniform float uHalfSize;
uniform vec3 uGridColor;
uniform vec3 uXAxisColor;
uniform vec3 uZAxisColor;
uniform mat4 viewProjectionMatrix;

const float fadeStart = 50.0;
const float fadeEnd   = 100.0;
const float gridLineWidth = 0.025;

float drawGrid(vec2 coord) {
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
    float line = min(grid.x, grid.y);
    return 1.0 - min(line, 1.0);
}

void main() {
    vec3 rayDir = normalize(worldPos_var - cameraPos);
    vec3 planeNormal = vec3(0.0, 1.0, 0.0);

    float plane_dot_ray = dot(planeNormal, rayDir);
    if (abs(plane_dot_ray) < 0.001 || sign(plane_dot_ray) == sign(cameraPos.y)) {
        discard;
    }

    float t = -cameraPos.y / rayDir.y;
    vec3 worldPos = cameraPos + rayDir * t;

    vec4 clipPos = viewProjectionMatrix * vec4(worldPos, 1.0);

    if (uInfinite == 0) {
        if (abs(worldPos.x) > uHalfSize || abs(worldPos.z) > uHalfSize) {
            discard;
        }
    }

    float fade = 1.0;
    if (uInfinite == 1) {
        float dist = length(worldPos - cameraPos);
        fade = 1.0 - smoothstep(fadeStart, fadeEnd, dist);
        if (fade < 0.01) discard;
    }

    float gridMajor = drawGrid(worldPos.xz);
    float gridMinor = drawGrid(worldPos.xz * 10.0) * 0.25;
    float gridLines = gridMajor + gridMinor * (1.0 - gridMajor);

    // Lighter grid color for dark background
    vec3 baseGridColor = vec3(0.35, 0.35, 0.38);
    vec3 finalColor = baseGridColor;
    
    // Colored axis lines
    if (abs(worldPos.z) < gridLineWidth) finalColor = uXAxisColor;
    if (abs(worldPos.x) < gridLineWidth) finalColor = uZAxisColor;

    float alpha = gridLines * fade * 0.6;
    if (abs(worldPos.z) < gridLineWidth || abs(worldPos.x) < gridLineWidth) {
        alpha = min(1.0, alpha + 0.5);
    }

    FragColor = vec4(finalColor, alpha);
    gl_FragDepth = clipPos.z / clipPos.w * 0.5 + 0.5;
}