#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Grid {
public:
	Grid(int size = 20, float spacing = 0.5f);
	~Grid();

	void initialize();
	void render(GLuint shader);

private:
	int m_size;
	float m_spacing;
	GLuint m_vao, m_vbo;
	std::vector<float> m_vertices;
	bool m_initialized;
};