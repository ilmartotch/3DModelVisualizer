#include "../Include/Grid.h"

Grid::Grid(int size, float spacing) :
    m_size(size),
    m_spacing(spacing),
    m_vao(0),
    m_vbo(0),
    m_initialized(false) {
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

    // Per memorizzare sia le posizioni che i colori
    struct Vertex {
        float position[3];
        float color[3];
    };

    std::vector<Vertex> gridVertices;

    //linee orizzontali e verticali
    for (int i = -m_size; i <= m_size; i++) {
        // Colori predefiniti
        float normalColor[3] = { 0.5f, 0.5f, 0.5f };     // Grigio per linee normali
        float xAxisColor[3] = { 0.8f, 0.2f, 0.2f };      // Rosso per asse X
        float zAxisColor[3] = { 0.2f, 0.2f, 0.8f };      // Blu per asse Z

        float* currentColor = normalColor;

        // Determina se questa è la linea dell'asse X (quando z = 0)
        if (i == 0) {
            currentColor = zAxisColor;
        }

        //linee orizzontali (parallele all'asse X)
        Vertex v1, v2;

        // Primo vertice
        v1.position[0] = -m_size * m_spacing;  // x1
        v1.position[1] = 0.0f;                // y1
        v1.position[2] = i * m_spacing;       // z1

        // Colora in rosso se è l'asse X (z = 0)
        if (i == 0) {
            memcpy(v1.color, xAxisColor, sizeof(xAxisColor));
        }
        else {
            memcpy(v1.color, normalColor, sizeof(normalColor));
        }

        // Secondo vertice
        v2.position[0] = m_size * m_spacing;  // x2
        v2.position[1] = 0.0f;                // y2
        v2.position[2] = i * m_spacing;       // z2

        // Lo stesso colore del primo vertice della linea
        memcpy(v2.color, v1.color, sizeof(v1.color));

        gridVertices.push_back(v1);
        gridVertices.push_back(v2);

        // Linee verticali (parallele all'asse Z)
        Vertex v3, v4;

        // Primo vertice
        v3.position[0] = i * m_spacing;       // x1
        v3.position[1] = 0.0f;                // y1
        v3.position[2] = -m_size * m_spacing; // z1

        // Colora in blu se è l'asse Z (x = 0)
        if (i == 0) {
            memcpy(v3.color, zAxisColor, sizeof(zAxisColor));
        }
        else {
            memcpy(v3.color, normalColor, sizeof(normalColor));
        }

        // Secondo vertice
        v4.position[0] = i * m_spacing;       // x2
        v4.position[1] = 0.0f;                // y2
        v4.position[2] = m_size * m_spacing;  // z2

        // Lo stesso colore del primo vertice della linea
        memcpy(v4.color, v3.color, sizeof(v3.color));

        gridVertices.push_back(v3);
        gridVertices.push_back(v4);
    }

    // Crea i buffer
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(Vertex), gridVertices.data(), GL_STATIC_DRAW);

    // Configurazione degli attributi per le posizioni (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Configurazione degli attributi per i colori (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Memorizza il numero di vertici per il rendering
    m_vertexCount = gridVertices.size();
    m_initialized = true;
}

void Grid::render(GLuint shader) {
    if (!m_initialized) return;
    {
        glUseProgram(shader);
        glBindVertexArray(m_vao);
        glDrawArrays(GL_LINES, 0, m_vertexCount);
        glBindVertexArray(0);
    }
}
