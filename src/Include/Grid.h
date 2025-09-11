#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Grid {
public:
    Grid(int lines, float spacing); // Costruttore con parametri aggiunto
    ~Grid();

    void initialize();
    void render(GLuint shader, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPosition);

private:
    GLuint m_vao;
    GLuint m_vbo;
    bool m_initialized;
    int m_lines;
    float m_spacing;
};