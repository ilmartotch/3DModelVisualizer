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

    int imageWidth = 0;
    int imageHeight = 0;
    float aspectRatio = 1.0f;
    bool hasAlpha = false;
    const float BASE_SIZE = 1.0f;
    
    void generateQuadGeometry();

public:
    ImagePlaneModel(const std::string& name = "ImagePlane");
    virtual ~ImagePlaneModel();
    
    virtual void initialize() override;
    virtual void render() override;
    virtual void cleanup() override;
    virtual std::shared_ptr<Model> clone() const override;
    virtual void setupVertexAttributes() override;
    
    void setImageDimensions(int width, int height);
    void setHasAlpha(bool alpha) { hasAlpha = alpha; }
    
    int getImageWidth() const { return imageWidth; }
    int getImageHeight() const { return imageHeight; }
    float getAspectRatio() const { return aspectRatio; }
    bool getHasAlpha() const { return hasAlpha; }

    void setImageSize(int width, int height, bool alpha) {
        setImageDimensions(width, height);
        setHasAlpha(alpha);
    }
};