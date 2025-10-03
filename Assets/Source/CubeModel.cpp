#include "../Include/CubeModel.h"
#include <glad/glad.h>
#include <memory>

CubeModel::CubeModel(const std::string& name) : Model(name) {
	m_initialized = false;
}

CubeModel::~CubeModel() {
	cleanup();
}

void CubeModel::initialize() {
    if (m_initialized) return;

    
    m_vertices = {
        // Posizione           // Normale
        // Faccia frontale
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 0
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 1
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 2
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 3
        // Faccia posteriore
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 4
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 5
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 6
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 7
        // Faccia sinistra
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, // 8
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, // 9
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, // 10
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, // 11
        // Faccia destra
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, // 12
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, // 13
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, // 14
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, // 15
        // Faccia inferiore
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, // 16
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, // 17
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, // 18
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, // 19
        // Faccia superiore
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, // 20
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, // 21
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, // 22
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f  // 23
    };

    m_indices = {
        0, 1, 2,   2, 3, 0,       // Front
        4, 5, 6,   6, 7, 4,       // Back
        8, 9, 10,  10, 11, 8,     // Left
        12, 13, 14, 14, 15, 12,   // Right
        16, 17, 18, 18, 19, 16,   // Bottom
        20, 21, 22, 22, 23, 20    // Top
    };

    // Creazione dei buffer OpenGL
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

void CubeModel::setupVertexAttributes() {
    GLsizei stride = 6 * sizeof(float);
    // Posizioni (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    // Normali (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void CubeModel::render() {
    if (!m_initialized) return;

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void CubeModel::cleanup() {
    if (!m_initialized) return;

    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    m_ebo = 0;

    m_initialized = false;
}

std::shared_ptr<Model> CubeModel::clone() const {
    auto newModel = std::make_shared<CubeModel>(*this);
    newModel->m_initialized = false; 
    newModel->initialize();
    return newModel;
}