#include "../Include/ImagePlaneModel.h"
#include <iostream>
#include <glm/glm.hpp>

ImagePlaneModel::ImagePlaneModel(const std::string& name) 
    : Model(name) {
    std::cout << "ImagePlaneModel creato: " << name << std::endl;
}

ImagePlaneModel::~ImagePlaneModel() {
    cleanup();
}

void ImagePlaneModel::setImageDimensions(int width, int height) {
    imageWidth = width;
    imageHeight = height;
    
    if (height > 0) {
        aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    } else {
        aspectRatio = 1.0f;
    }
    
    std::cout << "ImagePlane dimensions set: " << width << "x" << height 
              << " (aspect: " << aspectRatio << ")" << std::endl;
}

void ImagePlaneModel::generateQuadGeometry() {
    // Calcola dimensioni del quad mantenendo aspect ratio
    float quadWidth = BASE_SIZE * aspectRatio;
    float quadHeight = BASE_SIZE;
    
    // Centra il quad sull'origine
    float halfW = quadWidth * 0.5f;
    float halfH = quadHeight * 0.5f;
    
    // Vertici del quad (4 vertici, piano XY guardando verso +Z)
    float vertices[] = {
        // Posizioni (X, Y, Z)
        -halfW, -halfH, 0.0f,  // Bottom-left
         halfW, -halfH, 0.0f,  // Bottom-right
         halfW,  halfH, 0.0f,  // Top-right
        -halfW,  halfH, 0.0f   // Top-left
    };
    
    // Normali (tutte verso +Z)
    float normals[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    };
    
    // Coordinate texture (standard UV mapping)
    float texcoords[] = {
        0.0f, 0.0f,  // Bottom-left
        1.0f, 0.0f,  // Bottom-right
        1.0f, 1.0f,  // Top-right
        0.0f, 1.0f   // Top-left
    };
    
    // Indici (2 triangoli)
    unsigned int indices[] = {
        0, 1, 2,  // Primo triangolo
        2, 3, 0   // Secondo triangolo
    };
    
    // Genera VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // VBO vertici
    glGenBuffers(1, &VBO_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // VBO normali
    glGenBuffers(1, &VBO_normals);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_normals);
    glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    
    // VBO texcoords
    glGenBuffers(1, &VBO_texcoords);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_texcoords);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    
    // EBO
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glBindVertexArray(0);
    
    std::cout << "Quad geometry generated: " << quadWidth << "x" << quadHeight << std::endl;
}

void ImagePlaneModel::initialize() {
    if (m_initialized) {
        std::cout << "ImagePlaneModel già inizializzato: " << getName() << std::endl;
        return;
    }
    
    // Se non abbiamo dimensioni, usa default quadrato
    if (imageWidth == 0 || imageHeight == 0) {
        setImageDimensions(512, 512);
        std::cout << "Usando dimensioni default per " << getName() << std::endl;
    }
    
    generateQuadGeometry();
    m_initialized = true;
    
    std::cout << "ImagePlaneModel inizializzato: " << getName() << std::endl;
}

void ImagePlaneModel::render() {
    if (!m_initialized) {
        std::cerr << "ImagePlaneModel non inizializzato: " << getName() << std::endl;
        return;
    }
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void ImagePlaneModel::cleanup() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    
    if (VBO_vertices != 0) {
        glDeleteBuffers(1, &VBO_vertices);
        VBO_vertices = 0;
    }
    
    if (VBO_normals != 0) {
        glDeleteBuffers(1, &VBO_normals);
        VBO_normals = 0;
    }
    
    if (VBO_texcoords != 0) {
        glDeleteBuffers(1, &VBO_texcoords);
        VBO_texcoords = 0;
    }
    
    if (EBO != 0) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
    
    m_initialized = false;
    std::cout << "ImagePlaneModel cleanup completato: " << getName() << std::endl;
}

std::shared_ptr<Model> ImagePlaneModel::clone() const {
    auto newModel = std::make_shared<ImagePlaneModel>(getName());
    newModel->setImageDimensions(imageWidth, imageHeight);
    newModel->setHasAlpha(hasAlpha);
    
    // Copia texture se presente
    if (hasTexture()) {
        newModel->setTexture(getTextureID());
    }
    
    return newModel;
}

void ImagePlaneModel::setupVertexAttributes() {
    glBindVertexArray(VAO);
    
    // Attributo 0: Posizione
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    // Attributo 1: Normale
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    // Attributo 2: Coordinate texture
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}