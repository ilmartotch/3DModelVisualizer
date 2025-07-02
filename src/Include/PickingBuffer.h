#ifndef PICKING_BUFFER_H
#define PICKING_BUFFER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

// Gestisce il framebuffer per il picking basato su ID
class PickingBuffer {
public:
    PickingBuffer();
    ~PickingBuffer();

    // Inizializza il buffer di picking con le dimensioni specificate
    bool initialize(int width, int height);
    void cleanup();

    // Ridimensiona il buffer di picking
    void resize(int width, int height);

    // Attiva/disattiva il framebuffer
    void bind();
    void unbind();

    // Pulisce il buffer
    void clear();

    // Legge il colore ID al punto specificato
    glm::vec3 readPixel(int x, int y);

    // Getter
    GLuint getColorTextureId() const { return m_colorTextureId; }
    GLuint getPickingTextureId() const { return m_pickingTextureId; }
    GLuint getDepthTextureId() const { return m_depthTextureId; }
    GLuint getFramebufferId() const { return m_framebufferId; }

private:
    GLuint m_framebufferId;     // ID del FBO
    GLuint m_colorTextureId;    // Texture per colore visivo normale
    GLuint m_pickingTextureId;  // Texture per ID colore (picking)
    GLuint m_depthTextureId;    // Texture per depth buffer

    int m_width;
    int m_height;

    bool m_initialized;
};

#endif // PICKING_BUFFER_H
