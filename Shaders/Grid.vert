#version 330 core

layout (location = 0) in vec2 aPos; // A single quad vertex, values are -1 or 1

// We pass the world-space position of the ray's start (camera) and end points
out vec3 vs_world_ray_origin;
out vec3 vs_world_ray_end;

uniform mat4 u_inverse_projection_matrix;
uniform mat4 u_inverse_view_matrix;
uniform vec3 u_camera_pos;

void main() {
    // Calculate the world-space position of the fragment on the near and far planes
    vec4 near_clip = vec4(aPos, -1.0, 1.0);
    vec4 far_clip = vec4(aPos, 1.0, 1.0);

    vec4 near_view = u_inverse_projection_matrix * near_clip;
    vec4 far_view = u_inverse_projection_matrix * far_clip;

    near_view /= near_view.w;
    far_view /= far_view.w;

    // The ray starts at the camera position
    vs_world_ray_origin = u_camera_pos;
    // The ray "ends" at the unprojected far plane position
    vs_world_ray_end = (u_inverse_view_matrix * far_view).xyz;

    // Draw a full-screen quad at the far plane
    gl_Position = vec4(aPos, 0.99999, 1.0);
}