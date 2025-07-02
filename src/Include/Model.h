#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

//classe fot all 3D model
class Model {
public:
	Model(const std::string& name);
	virtual ~Model();

	//initialize and rendering
	virtual void initialize() = 0;
	virtual void render() = 0;
	virtual void cleanup() = 0;
	virtual void update(float currentTime);

	const std::string& getName() const { return m_name; }
	bool isInitialized() const { return m_initialized; }

	enum class RenderMode {
		SOLID = 0,
		WIREFRAME = 1,
		SOLID_WITH_WIREFRAME = 2
	};

	void setRenderMode(RenderMode mode) { m_renderMode = mode; }
	RenderMode getRenderMode() const { return m_renderMode; }

	//metodo per rendering istanziato 
	virtual void renderInstanced(int instanceCount) {
		if (!m_initialized) return;

		glBindVertexArray(m_vao);

		switch (m_renderMode) 
		{
		case Model::RenderMode::SOLID:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawElementsInstanced(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0, instanceCount);
			break;
		case Model::RenderMode::WIREFRAME:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawElementsInstanced(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0, instanceCount);
			break;
		case Model::RenderMode::SOLID_WITH_WIREFRAME:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawElementsInstanced(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0, instanceCount);

			// Poi renderizza il wireframe
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(-1.0f, -1.0f);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(1.5f);
			glDrawElementsInstanced(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0, instanceCount);
			glLineWidth(1.0f);
			glDisable(GL_POLYGON_OFFSET_FILL);
			break;
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(0);
	}

	GLuint getVAO() const { return m_vao; }

protected:
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_ebo;
	std::vector<float> m_vertices;
	std::vector<unsigned int> m_indices;

	std::string m_name;
	bool m_initialized;

	void updateColors(float currentTime);
	RenderMode m_renderMode = RenderMode::SOLID;
};