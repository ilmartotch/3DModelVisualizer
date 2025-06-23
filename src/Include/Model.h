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