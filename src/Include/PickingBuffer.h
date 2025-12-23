#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class PickingBuffer {
public:
    PickingBuffer();
    ~PickingBuffer();

    bool initialize(int width, int height);
    void cleanup();

    void resize(int width, int height);

    void bind();
    void unbind();

    void clear();

    glm::vec3 readPixel(int x, int y);

    bool isInitialized() const { return m_initialized; }

    GLuint getColorTextureId() const { return m_colorTextureId; }
    GLuint getPickingTextureId() const { return m_pickingTextureId; }
    GLuint getDepthTextureId() const { return m_depthTextureId; }
    GLuint getFramebufferId() const { return m_framebufferId; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    GLuint m_framebufferId;
    GLuint m_colorTextureId;
    GLuint m_pickingTextureId;
    GLuint m_depthTextureId;

    int m_width = 0;
    int m_height = 0;
    bool m_initialized = 0;
};