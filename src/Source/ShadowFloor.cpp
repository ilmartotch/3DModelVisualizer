#include "../Include/ShadowFloor.h"
#include <vector>
#include <glad/glad.h>

ShadowFloor::ShadowFloor(const std::string& name) : Model(name) {}

ShadowFloor::~ShadowFloor() {
    cleanup();
}

void ShadowFloor::initialize() {
    std::vector<float> vertices = {
        // positions          // normals           // texture coords
        -100.0f, 0.0f, -100.0f,  0.0f, 1.0f, 0.0f,   0.0f, 100.0f,
         100.0f, 0.0f, -100.0f,  0.0f, 1.0f, 0.0f,  100.0f, 100.0f,
         100.0f, 0.0f,  100.0f,  0.0f, 1.0f, 0.0f,  100.0f, 0.0f,
        -100.0f, 0.0f,  100.0f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,
        2, 3, 0
    };

    m_indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    setupVertexAttributes();

    glBindVertexArray(0);
}

void ShadowFloor::setupVertexAttributes() {
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

void ShadowFloor::render() {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void ShadowFloor::cleanup() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    m_vao = 0;
    m_vbo = 0;
    m_ebo = 0;
}

std::shared_ptr<Model> ShadowFloor::clone() const {
    return std::make_shared<ShadowFloor>(*this);
}