#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class Grid {
public:
    Grid();
    ~Grid();

    void initialize();
    void render(GLuint shader, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);
    void cleanup();

private:
    GLuint m_vao, m_vbo;
};