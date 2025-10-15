#pragma once

#include "../../src/Include/Model.h"
#include <glad/glad.h>
#include <string>
#include <memory>

class ImagePlaneModel : public Model {
private:
    GLuint VAO = 0;
    GLuint VBO_vertices = 0;
    GLuint VBO_normals = 0;
    GLuint VBO_texcoords = 0;
    GLuint EBO = 0;
    
    // Metadati immagine
    int imageWidth = 0;
    int imageHeight = 0;
    float aspectRatio = 1.0f;
    bool hasAlpha = false;
    
    // Dimensione base del plane (in unità world)
    const float BASE_SIZE = 1.0f;
    
    void generateQuadGeometry();

public:
    ImagePlaneModel(const std::string& name = "ImagePlane");
    virtual ~ImagePlaneModel();
    
    // Override metodi base Model
    virtual void initialize() override;
    virtual void render() override;
    virtual void cleanup() override;
    virtual std::shared_ptr<Model> clone() const override;
    virtual void setupVertexAttributes() override;
    
    // Metodi specifici per ImagePlane
    void setImageDimensions(int width, int height);
    void setHasAlpha(bool alpha) { hasAlpha = alpha; }
    
    // Getters
    int getImageWidth() const { return imageWidth; }
    int getImageHeight() const { return imageHeight; }
    float getAspectRatio() const { return aspectRatio; }
    bool getHasAlpha() const { return hasAlpha; }
};