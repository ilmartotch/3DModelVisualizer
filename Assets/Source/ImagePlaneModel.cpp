#include "../Include/ImagePlaneModel.h"
#include <glm/glm.hpp>

ImagePlaneModel::ImagePlaneModel(const std::string& name) 
    : Model(name) {
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
}

void ImagePlaneModel::generateQuadGeometry() {
    float quadWidth = BASE_SIZE * aspectRatio;
    float quadHeight = BASE_SIZE;
    
    float halfW = quadWidth * 0.5f;
    float halfH = quadHeight * 0.5f;

    float vertices[] = {
        -halfW, -halfH, 0.0f,  // Bottom-left
         halfW, -halfH, 0.0f,  // Bottom-right
         halfW,  halfH, 0.0f,  // Top-right
        -halfW,  halfH, 0.0f   // Top-left
    };
    
    float normals[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    };
    
    float texcoords[] = {
        0.0f, 0.0f,  // Bottom-left
        1.0f, 0.0f,  // Bottom-right
        1.0f, 1.0f,  // Top-right
        0.0f, 1.0f   // Top-left
    };
    
    unsigned int indices[] = {
        0, 1, 2,  // Primo triangolo
        2, 3, 0   // Secondo triangolo
    };
    
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &VBO_normals);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_normals);
    glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &VBO_texcoords);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_texcoords);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glBindVertexArray(0);
}

void ImagePlaneModel::initialize() {
    if (m_initialized) {
        return;
    }

    if (imageWidth == 0 || imageHeight == 0) {
        setImageDimensions(512, 512);
    }
    
    generateQuadGeometry();
    m_initialized = true;
}

void ImagePlaneModel::render() {
    if (!m_initialized) {
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
}

std::shared_ptr<Model> ImagePlaneModel::clone() const {
    auto newModel = std::make_shared<ImagePlaneModel>(getName());
    newModel->setImageDimensions(imageWidth, imageHeight);
    newModel->setHasAlpha(hasAlpha);

    if (hasTexture()) {
        newModel->setTexture(getTextureID());
    }
    
    return newModel;
}

void ImagePlaneModel::setupVertexAttributes() {
    glBindVertexArray(VAO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}