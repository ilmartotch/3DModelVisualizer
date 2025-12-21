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
        float stackAngle = std::numbers::pi_v<float> / 2.0f - i * stackStep;
        float xy = radius * std::cos(stackAngle);
        float z = radius * std::sin(stackAngle);

        for (int j = 0; j <= m_sectors; ++j) {
            float sectorAngle = j * sectorStep;

            float x = xy * std::cos(sectorAngle);
            float y = xy * std::sin(sectorAngle);

            float px = x;
            float py = z;
            float pz = y;

            float length = std::sqrt(px * px + py * py + pz * pz);
            float invLen = length > 0.0f ? 1.0f / length : 0.0f;
            float nx = px * invLen;
            float ny = py * invLen;
            float nz = pz * invLen;

            m_vertices.push_back(px);
            m_vertices.push_back(py);
            m_vertices.push_back(pz);

            m_vertices.push_back(nx);
            m_vertices.push_back(ny);
            m_vertices.push_back(nz);
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

        hasUVs = false;

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

void SphereModel::setupVertexAttributes() {
    
    GLsizei stride = hasUVs ? 8 * sizeof(float) : 6 * sizeof(float);

    // Posizioni (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // Normali (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // UV (location 2) - solo se presenti
    if (hasUVs) {
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
    else {
        // Disabilita l'attributo UV se non presente
        glDisableVertexAttribArray(2);
    }
}

std::shared_ptr<Model> SphereModel::clone() const {
    auto newModel = std::make_shared<SphereModel>(*this);
    newModel->initialize();
    return newModel;
}