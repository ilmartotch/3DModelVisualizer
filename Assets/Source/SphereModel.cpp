#include "../Include/SphereModel.h"
#include <numbers>

SphereModel::SphereModel(int sectors, int stacks, const std::string& name) : Model(name),
m_sectors(sectors),
m_stacks(stacks) {
}

SphereModel::~SphereModel() {
	cleanup();
}

void SphereModel::initialize() {
    if (m_initialized) return;

    // Generiamo i vertici della sfera
    float radius = 0.5f;
    float sectorStep = 2.0f * std::numbers::pi / m_sectors;
    float stackStep = std::numbers::pi / m_stacks;

    // Svuotiamo i vettori vertici e indici
    m_vertices.clear();
    m_indices.clear();

    // Generazione vertici
    for (int i = 0; i <= m_stacks; ++i) {
        float stackAngle = std::numbers::pi / 2.0f - i * stackStep;  // da pi/2 a -pi/2
        float xy = radius * cosf(stackAngle);           // r * cos(u)
        float z = radius * sinf(stackAngle);            // r * sin(u)

        // Aggiungere vertici di ogni stack
        for (int j = 0; j <= m_sectors; ++j) {
            float sectorAngle = j * sectorStep;  // da 0 a 2pi

            // Coordinate dei vertici
            float x = xy * cosf(sectorAngle);    // r * cos(u) * cos(v)
            float y = xy * sinf(sectorAngle);    // r * cos(u) * sin(v)

            // Normalizzare il vettore per ottenere colori basati sulla direzione
            float nx = x / radius;
            float ny = y / radius;
            float nz = z / radius;

            // Coordinate dei vertici
            m_vertices.push_back(x);
            m_vertices.push_back(z); // Invertiamo y e z per orientare meglio la sfera
            m_vertices.push_back(y);

            // Colori (basati sulla normale normalizzata)
            m_vertices.push_back(0.5f + 0.5f * nx);
            m_vertices.push_back(0.5f + 0.5f * ny);
            m_vertices.push_back(0.5f + 0.5f * nz);
        }
    }

    // Generazione indici
    for (int i = 0; i < m_stacks; ++i) {
        int k1 = i * (m_sectors + 1);
        int k2 = k1 + m_sectors + 1;

        for (int j = 0; j < m_sectors; ++j, ++k1, ++k2) {
            // 2 triangoli per faccia
            if (i != 0) {
                m_indices.push_back(k1);
                m_indices.push_back(k2);
                m_indices.push_back(k1 + 1);
            }

            if (i != (m_stacks - 1)) {
                m_indices.push_back(k1 + 1);
                m_indices.push_back(k2);
                m_indices.push_back(k2 + 1);
            }
        }
    }

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

void SphereModel::render() {
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

void SphereModel::cleanup() {
	if (!m_initialized) return;
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vbo);
	glDeleteBuffers(1, &m_ebo);
	m_initialized = false;
}