#include "../Include/Model.h"
#include <cmath>

// Inizializzazione della texture default statica
GLuint Model::defaultTextureID = 0;

void Model::initializeDefaultTexture() {
    if (defaultTextureID == 0) {
        // Crea un pixel grigio chiaro
        unsigned char pixelData[4] = {200, 200, 200, 255}; // RGBA: grigio chiaro
        
        glGenTextures(1, &defaultTextureID);
        glBindTexture(GL_TEXTURE_2D, defaultTextureID);
        
        // Imposta i parametri della texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        // Carica il singolo pixel come texture 1x1
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    }
}

Model::Model(const std::string& name) :
	m_vao(0),
	m_vbo(0),
	m_ebo(0),
	m_name(name),
	m_initialized(false),
	m_indexCount(0),
	textureID(0),
	useTexture(false),
	textureType(TextureType::NONE)
{
	// Assicurati che la texture di default sia creata
	initializeDefaultTexture();
}

Model::~Model() {
	cleanup();
}

void Model::cleanup() {
	if (m_vao != 0) {
		glDeleteVertexArrays(1, &m_vao);
		m_vao = 0;
	}
	if (m_vbo != 0) {
		glDeleteBuffers(1, &m_vbo);
		m_vbo = 0;
	}
	if (m_ebo !=0)
	{
		glDeleteBuffers(1, &m_ebo);
		m_ebo = 0;
	}
	m_initialized = false;
}

void Model::updateColors(float currentTime) {
	for (int i = 0; i < m_vertices.size() / 6; i++) {
		int colorOffset = i * 6 + 3;

		m_vertices[colorOffset] = 0.5f + 0.5f * sin(currentTime + i * 0.5f);
		m_vertices[colorOffset + 1] = 0.5f + 0.5f * sin(currentTime + i * 1.0f + 2.0f);
		m_vertices[colorOffset + 2] = 0.5f + 0.5f * sin(currentTime + i * 1.5f + 4.0f);
	}

	if (m_initialized) {
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(float), m_vertices.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void Model::update(float currentTime) {
	updateColors(currentTime);
}

void Model::render() {
	if (m_vao == 0) return;
	
	glBindVertexArray(m_vao);
	
	// Attiva la texture appropriata
	glActiveTexture(GL_TEXTURE0);
	if (useTexture && textureID != 0) {
		// Usa la texture assegnata all'oggetto
		glBindTexture(GL_TEXTURE_2D, textureID);
	} else {
		// Usa la texture default (grigio chiaro)
		glBindTexture(GL_TEXTURE_2D, defaultTextureID);
	}
	
	// Rendering con gli indici o senza
	if (m_indexCount > 0) {
		glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
	} else {
        GLsizei m_vertexCount = static_cast<GLsizei>(m_vertices.size() / 6);
        glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
	}
	
	// Unbind
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Model::setupBuffers() {
    if (m_vertices.empty()) return;

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);

    // Buffer dei vertici
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(float),
        m_vertices.data(), GL_STATIC_DRAW);

    // Se abbiamo indici, crea anche l'EBO
    if (!m_indices.empty()) {
        glGenBuffers(1, &m_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int),
            m_indices.data(), GL_STATIC_DRAW);
        m_indexCount = static_cast<GLsizei>(m_indices.size());
    } else {
        m_indexCount = 0;
    }

    // Configura gli attributi dei vertici
    setupVertexAttributes();

    glBindVertexArray(0);
}