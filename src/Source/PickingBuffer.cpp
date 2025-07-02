#include "../Include/PickingBuffer.h"
#include <iostream>

PickingBuffer::PickingBuffer()
    : m_framebufferId(0), m_colorTextureId(0), m_pickingTextureId(0), m_depthTextureId(0),
    m_width(0), m_height(0), m_initialized(false) {
}

PickingBuffer::~PickingBuffer() {
    cleanup();
}

bool PickingBuffer::initialize(int width, int height) {
    if (m_initialized) {
        cleanup();
    }

    m_width = width;
    m_height = height;

    // Genera il FBO
    glGenFramebuffers(1, &m_framebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);

    // 1. Crea texture per il color buffer normale
    glGenTextures(1, &m_colorTextureId);
    glBindTexture(GL_TEXTURE_2D, m_colorTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTextureId, 0);

    // 2. Crea texture per il buffer di picking (ID colore)
    glGenTextures(1, &m_pickingTextureId);
    glBindTexture(GL_TEXTURE_2D, m_pickingTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_pickingTextureId, 0);

    // 3. Crea texture per il depth buffer
    glGenTextures(1, &m_depthTextureId);
    glBindTexture(GL_TEXTURE_2D, m_depthTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTextureId, 0);

    // Configura multiple render targets (MRT)
    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    // Verifica completezza
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Errore: Framebuffer non completo!" << std::endl;
        cleanup();
        return false;
    }

    // Ripristina il framebuffer di default
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_initialized = true;
    return true;
}

void PickingBuffer::cleanup() {
    if (m_colorTextureId) {
        glDeleteTextures(1, &m_colorTextureId);
        m_colorTextureId = 0;
    }

    if (m_pickingTextureId) {
        glDeleteTextures(1, &m_pickingTextureId);
        m_pickingTextureId = 0;
    }

    if (m_depthTextureId) {
        glDeleteTextures(1, &m_depthTextureId);
        m_depthTextureId = 0;
    }

    if (m_framebufferId) {
        glDeleteFramebuffers(1, &m_framebufferId);
        m_framebufferId = 0;
    }

    m_initialized = false;
}

void PickingBuffer::resize(int width, int height) {
    if (m_width == width && m_height == height) {
        return;
    }

    // Reinizializza con le nuove dimensioni
    cleanup();
    initialize(width, height);
}

void PickingBuffer::bind() {
    if (!m_initialized) {
        std::cerr << "Errore: Tentativo di attivare un picking buffer non inizializzato!" << std::endl;
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
    glViewport(0, 0, m_width, m_height);
}

void PickingBuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PickingBuffer::clear() {
    if (!m_initialized) return;

    bind();

    // Cancella entrambi i buffer di colore con colore nero (nessun oggetto)
    float clearColorZero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Cancella il color attachment 0 (buffer colore normale)
    glClearBufferfv(GL_COLOR, 0, clearColorZero);

    // Cancella il color attachment 1 (buffer ID)
    glClearBufferfv(GL_COLOR, 1, clearColorZero);

    // Cancella il buffer di profondità
    glClear(GL_DEPTH_BUFFER_BIT);

    unbind();
}

glm::vec3 PickingBuffer::readPixel(int x, int y) {
    if (!m_initialized) {
        return glm::vec3(0.0f);
    }

    // Inverte la coordinata Y per adattarsi alle coordinate OpenGL
    y = m_height - y;

    // Verifica che le coordinate siano all'interno del framebuffer
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return glm::vec3(0.0f);
    }

    // Bind del framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);

    // Specifica che vogliamo leggere dal secondo attachment (ID buffer)
    glReadBuffer(GL_COLOR_ATTACHMENT1);

    // Legge il pixel
    float pixelData[4];
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, pixelData);

    // Ripristina il framebuffer di default
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Restituisce i primi 3 componenti come vettore (RGB)
    return glm::vec3(pixelData[0], pixelData[1], pixelData[2]);
}
