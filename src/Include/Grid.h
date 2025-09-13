#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class Grid {
public:
    Grid(float size);
    ~Grid();

    void initialize();
    void render(GLuint shader, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);
    void cleanup();

private:
    float m_size;
    GLuint m_vao, m_vbo;
};