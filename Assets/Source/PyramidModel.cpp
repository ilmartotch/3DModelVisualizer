#include "../Include/PyramidModel.h"

PyramidModel::PyramidModel(const std::string& name) : Model(name) {
	m_initialized = false;
}

PyramidModel::~PyramidModel() {
	cleanup();
}

void PyramidModel::initialize() {
	if (m_initialized) return;

	m_vertices = {
        // Posizioni                Colori
        -0.5f, 0.0f, -0.5f,         1.0f, 0.0f, 0.0f,  // 0: sinistra-back (rosso)
         0.5f, 0.0f, -0.5f,         0.0f, 1.0f, 0.0f,  // 1: destra-back (verde)
         0.5f, 0.0f,  0.5f,         0.0f, 0.0f, 1.0f,  // 2: destra-front (blu)
        -0.5f, 0.0f,  0.5f,         1.0f, 1.0f, 0.0f,  // 3: sinistra-front (giallo)
        // Apice
         0.0f, 0.8f,  0.0f,         1.0f, 0.0f, 1.0f   // 4: top (magenta)
	};

    //definizioni indici
    m_indices = {
        0, 1, 2,  2, 3, 0,     // base
        0, 1, 4,               // lato 1
        1, 2, 4,               // lato 2
        2, 3, 4,               // lato 3
        3, 0, 4                // lato 4
    };

    //buffer openGL
    glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_ebo);

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(float), m_vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

    //Posizioni
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Colori
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    m_initialized = true;
}

void PyramidModel::render() {
    if (!m_initialized) return;

    glBindVertexArray(m_vao);

    switch (m_renderMode) {
    case RenderMode::SOLID:
        // Modalità solida (default)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);
        break;

    case RenderMode::WIREFRAME:
        // Modalità wireframe
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);
        break;

    case RenderMode::SOLID_WITH_WIREFRAME:
        // Prima renderizza il solido
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);

        // Poi renderizza il wireframe
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(1.5f);
        glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);
        glLineWidth(1.0f);
        glDisable(GL_POLYGON_OFFSET_FILL);
        break;
    }

    // Ripristina lo stato
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
}

void PyramidModel::cleanup() {
	if (!m_initialized) return;
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vbo);
	glDeleteBuffers(1, &m_ebo);
	m_initialized = false;
}