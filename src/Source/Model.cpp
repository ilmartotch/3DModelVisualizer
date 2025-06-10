#include "../Include/Model.h"
#include <cmath>

Model::Model(const std::string& name) :
	m_vao(0),
	m_vbo(0),
	m_ebo(0),
	m_name(name),
	m_initialized(false) {
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