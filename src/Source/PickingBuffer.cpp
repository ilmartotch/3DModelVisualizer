#include "../Include/PickingBuffer.h"
#include <iostream>

PickingBuffer::PickingBuffer(){
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

    // Verifica completezza del framebuffer
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Errore: Framebuffer di picking non completo! Status: " << status << std::endl;

        // Log dettagliato dell'errore
        switch (status) {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER" << std::endl;
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            std::cerr << "GL_FRAMEBUFFER_UNSUPPORTED" << std::endl;
            break;
        default:
            std::cerr << "Errore sconosciuto del framebuffer" << std::endl;
            break;
        }

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

    m_width = 0;
    m_height = 0;
    m_initialized = false;
}

void PickingBuffer::resize(int width, int height) {
    if (m_width == width && m_height == height) {
        return;
    }

    // Reinizializza con le nuove dimensioni
    bool wasInitialized = m_initialized;
    cleanup();

    if (wasInitialized) {
        if (!initialize(width, height)) {
            std::cerr << "Errore nel ridimensionamento del buffer di picking!" << std::endl;
        }
    }
}

void PickingBuffer::bind() {
    if (!m_initialized) {
        std::cerr << "Errore: Tentativo di attivare un picking buffer non inizializzato!" << std::endl;
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
    glViewport(0, 0, m_width, m_height);

    // Assicurati che stiamo scrivendo su entrambi i color attachments
    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);
}

void PickingBuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PickingBuffer::clear() {
    if (!m_initialized) {
        std::cerr << "Errore: Tentativo di pulire un picking buffer non inizializzato!" << std::endl;
        return;
    }

    // Salva il framebuffer corrente
    GLint currentFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFramebuffer);

    // Bind del nostro framebuffer
    bind();

    // Cancella entrambi i buffer di colore con colore nero (nessun oggetto)
    float clearColorZero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Cancella il color attachment 0 (buffer colore normale)
    glClearBufferfv(GL_COLOR, 0, clearColorZero);

    // Cancella il color attachment 1 (buffer ID)
    glClearBufferfv(GL_COLOR, 1, clearColorZero);

    // Cancella il buffer di profondità
    glClear(GL_DEPTH_BUFFER_BIT);

    // Ripristina il framebuffer precedente
    glBindFramebuffer(GL_FRAMEBUFFER, currentFramebuffer);
}

glm::vec3 PickingBuffer::readPixel(int x, int y) {
    if (!m_initialized) {
        std::cerr << "[PICKING] Buffer non inizializzato!" << std::endl;
        return glm::vec3(0.0f);
    }

    // Valida prima dell'inversione
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        std::cerr << "[PICKING] Coordinate fuori range: (" << x << "," << y 
                  << ") su buffer " << m_width << "x" << m_height << std::endl;
        return glm::vec3(0.0f);
    }

    // Inverti Y dopo validazione
    int flippedY = m_height - y - 1;

    // Salva stati
    GLint currentFBO, currentReadBuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    glGetIntegerv(GL_READ_BUFFER, &currentReadBuffer);

    // Bind picking framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
    
    // Verifica completezza
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[PICKING] Framebuffer incompleto: " << status << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
        return glm::vec3(0.0f);
    }

    // Leggi dal COLOR_ATTACHMENT1 (picking texture)
    glReadBuffer(GL_COLOR_ATTACHMENT1);

    // Verifica errori SENZA nasconderli
    GLenum preError = glGetError();
    if (preError != GL_NO_ERROR) {
        std::cerr << "[PICKING] Errore OpenGL pre-esistente: " << preError << std::endl;
    }

    // Leggi pixel
    float pixelData[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glReadPixels(x, flippedY, 1, 1, GL_RGBA, GL_FLOAT, pixelData);

    // Controlla errori lettura
    GLenum postError = glGetError();
    if (postError != GL_NO_ERROR) {
        std::cerr << "[PICKING] Errore glReadPixels: " << postError 
                  << " a (" << x << "," << flippedY << ")" << std::endl;
    }

    // Ripristina stati
    glReadBuffer(currentReadBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);

    #ifdef DEBUG_PICKING
    std::cout << "[PICKING] Pixel(" << x << "," << y << ")→(" << x << "," << flippedY 
              << ") = RGB(" << pixelData[0] << "," << pixelData[1] << "," << pixelData[2] << ")" << std::endl;
    #endif

    return glm::vec3(pixelData[0], pixelData[1], pixelData[2]);
}