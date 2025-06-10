#include "../src/Include/Grid.h"

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

	//linee orizzontali e verticali
	for (int i = -m_size; i <= m_size; i++) {
		//linee orizzontali
		m_vertices.push_back(-m_size * m_spacing); // x1
		m_vertices.push_back(0.0f);               // y1
		m_vertices.push_back(i * m_spacing);      // z1

		m_vertices.push_back(m_size * m_spacing);  // x2
		m_vertices.push_back(0.0f);               // y2
		m_vertices.push_back(i * m_spacing);      // z2

		// Linee verticali
		m_vertices.push_back(i * m_spacing);      // x1
		m_vertices.push_back(0.0f);               // y1
		m_vertices.push_back(-m_size * m_spacing); // z1

		m_vertices.push_back(i * m_spacing);      // x2
		m_vertices.push_back(0.0f);               // y2
		m_vertices.push_back(m_size * m_spacing);  // z2
	}

	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(float), m_vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	m_initialized = true;
}

void Grid::render(GLuint shader) {
	if (!m_initialized) return;
	{
		glUseProgram(shader);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_LINES, 0, m_vertices.size() / 3);
		glBindVertexArray(0);
	}
}