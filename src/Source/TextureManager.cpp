#include "TextureManager.h"
#include <stb_image.h>

TextureManager::TextureManager() : m_defaultTexture(0) {
    getDefaultTexture();
}

TextureManager::~TextureManager() {
    cleanup();
}

GLuint TextureManager::loadTexture(const std::string& path) {
    // Return cached texture if already loaded
    if (m_textureMap.find(path) != m_textureMap.end()) {
        return m_textureMap[path];
    }

    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else {
            std::cerr << "Unsupported image format: " << nrComponents << " components." << std::endl;
            stbi_image_free(data);
            return 0;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        m_textureMap[path] = textureID;
    }
    else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }

    return textureID;
}

GLuint TextureManager::createColorTexture(const glm::vec4& color) {
    if (m_colorTextureMap.find(color) != m_colorTextureMap.end()) {
        return m_colorTextureMap[color];
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    unsigned char data[] = {
        static_cast<unsigned char>(color.r * 255.0f),
        static_cast<unsigned char>(color.g * 255.0f),
        static_cast<unsigned char>(color.b * 255.0f),
        static_cast<unsigned char>(color.a * 255.0f)
    };

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_colorTextureMap[color] = textureID;
    return textureID;
}

GLuint TextureManager::getDefaultTexture() {
    if (m_defaultTexture == 0) {
        m_defaultTexture = createColorTexture(glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
    }
    return m_defaultTexture;
}

void TextureManager::cleanup() {
    for (const auto& [path, id] : m_textureMap) {
        glDeleteTextures(1, &id);
    }
    m_textureMap.clear();

    for (const auto& [color, id] : m_colorTextureMap) {
        glDeleteTextures(1, &id);
    }
    m_colorTextureMap.clear();

    if (m_defaultTexture != 0) {
        glDeleteTextures(1, &m_defaultTexture);
        m_defaultTexture = 0;
    }
}

/*
TextureManager is a singleton that handles texture loading and caching.

Features:
- loadTexture loads from disk and caches by path to avoid duplicates
- createColorTexture creates 1x1 solid color textures for materials
- getDefaultTexture provides a light gray fallback for objects without textures

File textures are cached by absolute path string. Color textures are cached
by glm::vec4 using a custom hash function.

cleanup deletes all OpenGL textures and is called automatically in the
destructor. Can also be called manually before shutdown.

Supported formats: PNG, JPG, BMP, TGA via stb_image.
*/