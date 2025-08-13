#pragma once

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

    // Verifica se il buffer è stato inizializzato
    bool isInitialized() const { return m_initialized; }

    // Getter
    GLuint getColorTextureId() const { return m_colorTextureId; }
    GLuint getPickingTextureId() const { return m_pickingTextureId; }
    GLuint getDepthTextureId() const { return m_depthTextureId; }
    GLuint getFramebufferId() const { return m_framebufferId; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    GLuint m_framebufferId;     // ID del FBO
    GLuint m_colorTextureId;    // Texture per colore visivo normale
    GLuint m_pickingTextureId;  // Texture per ID colore (picking)
    GLuint m_depthTextureId;    // Texture per depth buffer

    int m_width = 0;                // Larghezza del buffer
    int m_height = 0;               // Altezza del buffer
    bool m_initialized = 0;         // Flag di inizializzazione
};