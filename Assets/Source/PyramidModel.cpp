#include "../Include/PyramidModel.h"
#include <glad/glad.h>
#include <memory>

PyramidModel::PyramidModel(const std::string& name) : Model(name) {
	m_initialized = false;
}

PyramidModel::~PyramidModel() {
	cleanup();
}

void PyramidModel::initialize() {
	if (m_initialized) return;

	m_vertices = {
        -0.5f, 0.0f, -0.5f,  0.0f, -1.0f,  0.0f, // 0
         0.5f, 0.0f, -0.5f,  0.0f, -1.0f,  0.0f, // 1
         0.5f, 0.0f,  0.5f,  0.0f, -1.0f,  0.0f, // 2
        -0.5f, 0.0f,  0.5f,  0.0f, -1.0f,  0.0f, // 3

        -0.5f, 0.0f,  0.5f,  0.0f,  0.447f, 0.894f, // 4
         0.5f, 0.0f,  0.5f,  0.0f,  0.447f, 0.894f, // 5
         0.0f, 0.8f,  0.0f,  0.0f,  0.447f, 0.894f, // 6 (Apice)

         0.5f, 0.0f,  0.5f,  0.894f, 0.447f, 0.0f, // 7
         0.5f, 0.0f, -0.5f,  0.894f, 0.447f, 0.0f, // 8
         0.0f, 0.8f,  0.0f,  0.894f, 0.447f, 0.0f, // 9 (Apice)

         0.5f, 0.0f, -0.5f,  0.0f,  0.447f, -0.894f, // 10
        -0.5f, 0.0f, -0.5f,  0.0f,  0.447f, -0.894f, // 11
         0.0f, 0.8f,  0.0f,  0.0f,  0.447f, -0.894f, // 12 (Apice)

        -0.5f, 0.0f, -0.5f, -0.894f, 0.447f, 0.0f, // 13
        -0.5f, 0.0f,  0.5f, -0.894f, 0.447f, 0.0f, // 14
         0.0f, 0.8f,  0.0f, -0.894f, 0.447f, 0.0f  // 15 (Apice)
	};

    m_indices = {
        // Base
        0, 1, 2,   2, 3, 0,
        // Lati
        4, 5, 6,
        7, 8, 9,
        10, 11, 12,
        13, 14, 15
    };

    hasUVs = false;

    glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(float), m_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

	setupVertexAttributes();

    glBindVertexArray(0);
    m_initialized = true;
}

void PyramidModel::setupVertexAttributes() {
    GLsizei stride = hasUVs ? 8 * sizeof(float) : 6 * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    if (hasUVs) {
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
    else {
        glDisableVertexAttribArray(2);
    }
}

void PyramidModel::render() {
    if (!m_initialized) return;

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void PyramidModel::cleanup() {
	if (!m_initialized) return;
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    m_ebo = 0;
	m_initialized = false;
}

std::shared_ptr<Model> PyramidModel::clone() const {
    auto newModel = std::make_shared<PyramidModel>(*this);
    newModel->initialize();
    return newModel;
}