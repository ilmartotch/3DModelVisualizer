#include "../Include/CubeModel.h"
#include <glad/glad.h>

CubeModel::CubeModel(const std::string& name) : Model(name) {
	m_initialized = false;
}

CubeModel::~CubeModel() {
	cleanup();
}

void CubeModel::initialize() {
    if (m_initialized) return;

    // Definizione dei vertici del cubo con colori
    m_vertices = {
        // Posizioni                Colori
        // Faccia frontale (z positivo)
        -0.5f, -0.5f,  0.5f,        1.0f, 0.0f, 0.0f,  // 0: in basso a sinistra
         0.5f, -0.5f,  0.5f,        0.0f, 1.0f, 0.0f,  // 1: in basso a destra
         0.5f,  0.5f,  0.5f,        0.0f, 0.0f, 1.0f,  // 2: in alto a destra
        -0.5f,  0.5f,  0.5f,        1.0f, 1.0f, 0.0f,  // 3: in alto a sinistra

        // Faccia posteriore (z negativo)
        -0.5f, -0.5f, -0.5f,        0.0f, 1.0f, 1.0f,  // 4: in basso a sinistra
         0.5f, -0.5f, -0.5f,        1.0f, 0.0f, 1.0f,  // 5: in basso a destra
         0.5f,  0.5f, -0.5f,        0.5f, 0.5f, 0.0f,  // 6: in alto a destra
        -0.5f,  0.5f, -0.5f,        0.0f, 0.5f, 0.5f,  // 7: in alto a sinistra
    };

    // Indici per disegnare le facce del cubo
    m_indices = {
        // Faccia frontale
        0, 1, 2,
        2, 3, 0,

        // Faccia destra
        1, 5, 6,
        6, 2, 1,

        // Faccia posteriore
        5, 4, 7,
        7, 6, 5,

        // Faccia sinistra
        4, 0, 3,
        3, 7, 4,

        // Faccia superiore
        3, 2, 6,
        6, 7, 3,

        // Faccia inferiore
        4, 5, 1,
        1, 0, 4
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

    // Posizioni
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Colori
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    m_initialized = true;
}

void CubeModel::render() {
    if (!m_initialized) return;

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void CubeModel::cleanup() {
	if (!m_initialized) return;
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vbo);
	glDeleteBuffers(1, &m_ebo);
	m_initialized = false;
}