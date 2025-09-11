#include "../Include/Grid.h"
#include <glm/gtc/type_ptr.hpp>

Grid::Grid(int lines, float spacing) :
    m_vao(0),
    m_vbo(0),
    m_initialized(false),
    m_lines(lines),
    m_spacing(spacing)
{
}

Grid::~Grid() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
    }
}

void Grid::initialize() {
    if (m_initialized) return;

    // A simple quad that fills the screen in clip space
    const float quad_vertices[] = {
        // positions
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
         1.0f, -1.0f,
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_initialized = true;
}

void Grid::render(GLuint shader, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPosition) {
    if (!m_initialized) return;

    glUseProgram(shader);

    // Calculate inverse matrices
    glm::mat4 inverse_projection = glm::inverse(projection);
    glm::mat4 inverse_view = glm::inverse(view);

    // Set shader uniforms
    glUniformMatrix4fv(glGetUniformLocation(shader, "u_inverse_projection_matrix"), 1, GL_FALSE, glm::value_ptr(inverse_projection));
    glUniformMatrix4fv(glGetUniformLocation(shader, "u_inverse_view_matrix"), 1, GL_FALSE, glm::value_ptr(inverse_view));
    glUniform3fv(glGetUniformLocation(shader, "u_camera_pos"), 1, glm::value_ptr(cameraPosition));

    // Grid properties
    glUniform3f(glGetUniformLocation(shader, "u_grid_color"), 0.5f, 0.5f, 0.5f); // Grey color
    glUniform1f(glGetUniformLocation(shader, "u_grid_scale"), 1.0f);           // Spacing of 1 unit
    glUniform1f(glGetUniformLocation(shader, "u_grid_line_width"), 0.02f);      // Line width

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Disable depth writing to ensure it doesn't obscure objects behind it
    glDepthMask(GL_FALSE);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    // Re-enable depth writing for the rest of the scene
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}