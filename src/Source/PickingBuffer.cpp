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

    glGenFramebuffers(1, &m_framebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);

    glGenTextures(1, &m_colorTextureId);
    glBindTexture(GL_TEXTURE_2D, m_colorTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTextureId, 0);

    glGenTextures(1, &m_pickingTextureId);
    glBindTexture(GL_TEXTURE_2D, m_pickingTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_pickingTextureId, 0);

    glGenTextures(1, &m_depthTextureId);
    glBindTexture(GL_TEXTURE_2D, m_depthTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTextureId, 0);

    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Errore: Framebuffer di picking non completo! Status: " << status << std::endl;

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

    GLint currentFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFramebuffer);

    bind();

    float clearColorZero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    glClearBufferfv(GL_COLOR, 0, clearColorZero);
    glClearBufferfv(GL_COLOR, 1, clearColorZero);
    glClear(GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, currentFramebuffer);
}

glm::vec3 PickingBuffer::readPixel(int x, int y) {
    if (!m_initialized) {
        std::cerr << "[PICKING] Buffer non inizializzato!" << std::endl;
        return glm::vec3(0.0f);
    }

    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        std::cerr << "[PICKING] Coordinate fuori range: (" << x << "," << y 
                  << ") su buffer " << m_width << "x" << m_height << std::endl;
        return glm::vec3(0.0f);
    }

    int flippedY = m_height - y - 1;

    GLint currentFBO, currentReadBuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    glGetIntegerv(GL_READ_BUFFER, &currentReadBuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[PICKING] Framebuffer incompleto: " << status << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
        return glm::vec3(0.0f);
    }

    glReadBuffer(GL_COLOR_ATTACHMENT1);

    GLenum preError = glGetError();
    if (preError != GL_NO_ERROR) {
        std::cerr << "[PICKING] Errore OpenGL pre-esistente: " << preError << std::endl;
    }

    float pixelData[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glReadPixels(x, flippedY, 1, 1, GL_RGBA, GL_FLOAT, pixelData);

    GLenum postError = glGetError();
    if (postError != GL_NO_ERROR) {
        std::cerr << "[PICKING] Errore glReadPixels: " << postError 
                  << " a (" << x << "," << flippedY << ")" << std::endl;
    }

    glReadBuffer(currentReadBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);

    #ifdef DEBUG_PICKING
    std::cout << "[PICKING] Pixel(" << x << "," << y << ")→(" << x << "," << flippedY 
              << ") = RGB(" << pixelData[0] << "," << pixelData[1] << "," << pixelData[2] << ")" << std::endl;
    #endif

    return glm::vec3(pixelData[0], pixelData[1], pixelData[2]);
}