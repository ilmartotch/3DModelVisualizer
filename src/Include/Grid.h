#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class Grid {
public:
    Grid();
    ~Grid();

    void initialize();
    void render(GLuint shader,
        const glm::mat4& projection,
        const glm::mat4& view,
        const glm::vec3& cameraPos,
        bool infiniteGrid,
        float halfSize,
        const glm::vec3& gridColor,
        const glm::vec3& xAxisColor,
        const glm::vec3& zAxisColor);
    void cleanup();

private:
    GLuint m_vao, m_vbo;
};