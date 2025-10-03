#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <memory>

// Enum per i tipi di texture supportati
enum class TextureType {
    IMAGE,    
    SOLID_COLOR, 
    DIFFUSE,
    SPECULAR,
    NORMAL,
    HEIGHT,
	RAINBOW,
};

class TextureManager {
public:
    static TextureManager& getInstance() {
        static TextureManager instance;
        return instance;
    }
    
    // Carica texture da file immagine
    GLuint loadTextureFromFile(const std::string& filepath);
    
    // Crea texture da colore uniforme
    GLuint createColorTexture(const glm::vec4& color);
    
    // Ottiene l'ID texture esistente o ne crea uno nuovo
    GLuint getTexture(const std::string& id);
    
    // Imposta parametri shader per texture arcobaleno
    void setupRainbowEffect(GLuint shader, float time, float speed = 1.0f);
    
    // Pulisci le risorse
    void cleanup();
    
private:
    TextureManager() = default;
    ~TextureManager() { cleanup(); }
    
    // Cache delle texture per riuso
    std::unordered_map<std::string, GLuint> textureCache;
    std::unordered_map<GLuint, GLuint> colorTextureCache;
};