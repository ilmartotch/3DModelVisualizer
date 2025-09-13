#include "../Include/Grid.h"
#include <vector>

Grid::Grid(float size) : m_size(size), m_vao(0), m_vbo(0) {}

Grid::~Grid() {
    cleanup();
}

void Grid::initialize() {
    // Vertici di un grande quad sul piano XZ (y=0)
    std::vector<float> vertices = {
        // Triangolo 1
        -m_size, 0.0f, -m_size,
         m_size, 0.0f, -m_size,
         m_size, 0.0f,  m_size,
         // Triangolo 2
          m_size, 0.0f,  m_size,
         -m_size, 0.0f,  m_size,
         -m_size, 0.0f, -m_size
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // L'attributo della posizione è a location 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Grid::render(GLuint shader, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) {
    glUseProgram(shader);

    // Imposta le matrici e la posizione della camera
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniform3fv(glGetUniformLocation(shader, "cameraPos"), 1, &cameraPos[0]);

    // Imposta il colore della griglia (opzionale, lo shader ha un default)
    glUniform3f(glGetUniformLocation(shader, "gridColor"), 0.5f, 0.5f, 0.5f);

    // Abilita il blending per la trasparenza e la dissolvenza
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disabilita la scrittura sul depth buffer per evitare che la griglia "copra" gli oggetti dietro di essa
    glDepthMask(GL_FALSE);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6); // Disegniamo 6 vertici (due triangoli)
    glBindVertexArray(0);

    // Ripristina gli stati di OpenGL
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Grid::cleanup() {
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}