#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <iostream>
#include <functional>

struct vec4_hash {
    std::size_t operator()(const glm::vec4& v) const {
        return std::hash<float>()(v.x) ^
            (std::hash<float>()(v.y) << 1) ^
            (std::hash<float>()(v.z) << 2) ^
            (std::hash<float>()(v.w) << 3);
    }
};

class TextureManager {
public:
    static TextureManager& getInstance() {
        static TextureManager instance;
        return instance;
    }

    GLuint loadTexture(const std::string& path);

    GLuint createColorTexture(const glm::vec4& color);

    GLuint getDefaultTexture();

    void cleanup();

private:
    TextureManager();
    ~TextureManager();

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    std::unordered_map<std::string, GLuint> m_textureMap;
    std::unordered_map<glm::vec4, GLuint, vec4_hash> m_colorTextureMap;
    GLuint m_defaultTexture;
};

#endif