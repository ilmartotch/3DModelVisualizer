#version 330 core

out vec4 FragColor;

in vec3 vs_world_ray_origin;
in vec3 vs_world_ray_end;

uniform vec3 u_grid_color;
uniform float u_grid_scale;
uniform float u_grid_line_width;

// Computes the intersection of a ray with a plane
// Returns the distance from the ray origin to the intersection point
float intersect_plane(vec3 origin, vec3 direction, vec3 normal, float d) {
    return -(dot(origin, normal) + d) / dot(direction, normal);
}

// Computes the opacity of the grid lines based on the distance from the camera
float compute_grid_opacity(float dist, float max_dist) {
    return 1.0 - min(dist / max_dist, 1.0);
}

void main() {
    vec3 ray_origin = vs_world_ray_origin;
    vec3 ray_direction = normalize(vs_world_ray_end - vs_world_ray_origin);

    // Define the ground plane (y=0)
    vec3 plane_normal = vec3(0.0, 1.0, 0.0);
    float plane_d = 0.0;

    // Find the distance to the intersection point
    float t = intersect_plane(ray_origin, ray_direction, plane_normal, plane_d);

    // If the intersection is behind the camera, discard the fragment
    if (t < 0.0) {
        discard;
    }

    // Find the world-space position of the intersection
    vec3 world_pos = ray_origin + ray_direction * t;

    // Determine which grid lines to draw

    // --- Primary Grid (every 1 unit) ---
    vec2 grid_pos_primary = world_pos.xz / u_grid_scale;
    vec2 grid_deriv_primary = dFdx(grid_pos_primary) + dFdy(grid_pos_primary);
    vec2 grid_line_width_primary = u_grid_line_width / (2.0 * u_grid_scale) * grid_deriv_primary;
    vec2 grid_line_primary = abs(fract(grid_pos_primary) - 0.5);
    vec2 grid_line_draw = smoothstep(vec2(0.5) - grid_line_width, vec2(0.5), grid_line);
    float primary_lines = max(grid_line_draw_primary.x, grid_line_draw_primary.y);

    // --- Secondary Grid (every 10 units, thicker) ---
    vec2 grid_pos_secondary = world_pos.xz / (u_grid_scale * 10.0);
    vec2 grid_deriv_secondary = dFdx(grid_pos_secondary) + dFdy(grid_pos_secondary);
    // Use a larger width for the secondary lines (e.g., 2.5x thicker)
    vec2 grid_line_width_secondary = (u_grid_line_width * 2.5) / (2.0 * u_grid_scale * 10.0) * grid_deriv_secondary;
    vec2 grid_line_secondary = abs(fract(grid_pos_secondary) - 0.5);
    vec2 grid_line_draw_secondary = smoothstep(vec2(0.5) - grid_line_width_secondary, vec2(0.5), grid_line_secondary);
    float secondary_lines = max(grid_line_draw_secondary.x, grid_line_draw_secondary.y);

    // Combine the lines and compute the final opacity
    float grid_lines = max(primary_lines, secondary_lines);
    float opacity = compute_grid_opacity(t, 400.0); // Fade out after 400 units

    // Final color
    FragColor = vec4(u_grid_color, grid_lines * opacity);
}