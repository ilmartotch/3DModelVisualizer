#pragma once
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string.h> // Per memcpy

class Grid {
public:
    Grid(int size = 20, float spacing = 0.5f);
    ~Grid();

    void initialize();
    void render(GLuint shader);

private:
    int m_size;
    float m_spacing;
    GLuint m_vao;
    GLuint m_vbo;
    bool m_initialized;
    size_t m_vertexCount;
};