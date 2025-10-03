#include "../src/Include/TextureManager.h"
#include <iostream>
#include <stb_image.h>

GLuint TextureManager::loadTextureFromFile(const std::string& filepath) {
    // Verifica se la texture è già caricata
    auto it = textureCache.find(filepath);
    if (it != textureCache.end()) {
        return it->second;
    }
    
    // Carica la texture con STB Image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        return 0;
    }
    
    // Determina il formato in base ai canali
    GLenum format;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;
    else {
        std::cerr << "Unsupported number of channels: " << channels << std::endl;
        stbi_image_free(data);
        return 0;
    }
    
    // Genera la texture OpenGL
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Imposta parametri della texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Carica l'immagine nella texture
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    stbi_image_free(data);
    
    // Salva nella cache e restituisci
    textureCache[filepath] = textureID;
    return textureID;
}

GLuint TextureManager::createColorTexture(const glm::vec4& color) {
    // Genera un hash semplice del colore
    GLuint colorHash = 
        static_cast<GLuint>(color.r * 255) << 24 | 
        static_cast<GLuint>(color.g * 255) << 16 | 
        static_cast<GLuint>(color.b * 255) << 8 | 
        static_cast<GLuint>(color.a * 255);
        
    // Verifica se esiste già
    auto it = colorTextureCache.find(colorHash);
    if (it != colorTextureCache.end()) {
        return it->second;
    }
    
    // Crea una texture 1x1 con il colore specificato
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Un singolo pixel del colore desiderato
    unsigned char pixelData[4] = {
        static_cast<unsigned char>(color.r * 255),
        static_cast<unsigned char>(color.g * 255),
        static_cast<unsigned char>(color.b * 255),
        static_cast<unsigned char>(color.a * 255)
    };
    
    // Carica il pixel come texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    
    // Imposta i parametri della texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Salva nella cache e restituisci
    colorTextureCache[colorHash] = textureID;
    return textureID;
}

GLuint TextureManager::getTexture(const std::string& id)
{
    return GLuint();
}

void TextureManager::setupRainbowEffect(GLuint shader, float time, float speed) {
    // Imposta le uniform per l'effetto arcobaleno
    glUseProgram(shader);
    glUniform1f(glGetUniformLocation(shader, "time"), time);
    glUniform1f(glGetUniformLocation(shader, "rainbowSpeed"), speed);
    glUniform1i(glGetUniformLocation(shader, "useRainbow"), 1);
}

void TextureManager::cleanup() {
    // Elimina tutte le texture nella cache
    for (const auto& pair : textureCache) {
        glDeleteTextures(1, &pair.second);
    }
    
    for (const auto& pair : colorTextureCache) {
        glDeleteTextures(1, &pair.second);
    }
    
    textureCache.clear();
    colorTextureCache.clear();
}