#include "../Include/Grid.h"
#include <vector>
#include <glm/gtc/type_ptr.hpp>

Grid::Grid() : m_vao(0), m_vbo(0) {}

Grid::~Grid() {
    cleanup();
}

void Grid::initialize() {
    // Vertici di un semplice quad che copre lo schermo in NDC
    std::vector<float> vertices = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Grid::render(GLuint shader,
                  const glm::mat4& projection,
                  const glm::mat4& view,
                  const glm::vec3& cameraPos,
                  bool infiniteGrid,
                  float halfSize,
                  const glm::vec3& gridColor,
                  const glm::vec3& xAxisColor,
                  const glm::vec3& zAxisColor) {
    glUseProgram(shader);

    // Matrici inverse
    glm::mat4 invView = glm::inverse(view);
    glm::mat4 invProjection = glm::inverse(projection);
	glm::mat4 viewProjectionMatrix = projection * view;

    glUniformMatrix4fv(glGetUniformLocation(shader, "invView"), 1, GL_FALSE, glm::value_ptr(invView));
    glUniformMatrix4fv(glGetUniformLocation(shader, "invProjection"), 1, GL_FALSE, glm::value_ptr(invProjection));
    glUniformMatrix4fv(glGetUniformLocation(shader, "viewProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(viewProjectionMatrix));
    glUniform3fv(glGetUniformLocation(shader, "cameraPos"), 1, &cameraPos[0]);

    // Uniform modalità/parametri griglia
    glUniform1i(glGetUniformLocation(shader, "uInfinite"), infiniteGrid ? 1 : 0);
    glUniform1f(glGetUniformLocation(shader, "uHalfSize"), halfSize);
    glUniform3fv(glGetUniformLocation(shader, "uGridColor"), 1, &gridColor[0]);
    glUniform3fv(glGetUniformLocation(shader, "uXAxisColor"), 1, &xAxisColor[0]);
    glUniform3fv(glGetUniformLocation(shader, "uZAxisColor"), 1, &zAxisColor[0]);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

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