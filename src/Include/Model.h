#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include "../Include/TextureManager.h"

//classe per tutti i modelli 3D
class Model {
public:
    Model(const std::string& name);
    virtual ~Model();

    //initialize and rendering
    virtual void initialize() = 0;
    virtual void render() = 0;
    virtual void cleanup() = 0;
    virtual void update(float currentTime);
    virtual void setupVertexAttributes() = 0;

    const std::string& getName() const { return m_name; }
    bool isInitialized() const { return m_initialized; }

    enum class RenderMode {
        SOLID = 0,
        WIREFRAME = 1,
        SOLID_WITH_WIREFRAME = 2
    };

    void setRenderMode(RenderMode mode) { m_renderMode = mode; }
    RenderMode getRenderMode() const { return m_renderMode; }

    enum class TextureType {
        NONE,          // Nessuna texture, usa colore default
        IMAGE,         // Texture da file immagine
        SOLID_COLOR    // Colore uniforme
    };

    //metodo per rendering istanziato 
    virtual void renderInstanced(int instanceCount) {
        if (!m_initialized) return;

        glBindVertexArray(m_vao);

        switch (m_renderMode)
        {
        case Model::RenderMode::SOLID:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            // Usa glDrawElementsInstanced se il modello ha indici, altrimenti glDrawArraysInstanced
            if (!m_indices.empty()) {
                glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0, instanceCount);
            }
            else {
                // Calcola il numero di vertici dal buffer dei vertici (assumendo 3 componenti per vertice)
                GLsizei vertexCount = static_cast<GLsizei>(m_vertices.size() / 6);
                glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, instanceCount);
            }
            break;

        case Model::RenderMode::WIREFRAME:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            if (!m_indices.empty()) {
                glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0, instanceCount);
            }
            else {
                GLsizei vertexCount = static_cast<GLsizei>(m_vertices.size() / 6);
                glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, instanceCount);
            }
            break;

        case Model::RenderMode::SOLID_WITH_WIREFRAME:
            // Prima renderizza il solido
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            if (!m_indices.empty()) {
                glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0, instanceCount);
            }
            else {
                GLsizei vertexCount = static_cast<GLsizei>(m_vertices.size() / 6);
                glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, instanceCount);
            }

            // Poi renderizza il wireframe con offset per evitare z-fighting
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.0f, -1.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(1.5f);

            if (!m_indices.empty()) {
                glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0, instanceCount);
            }
            else {
                GLsizei vertexCount = static_cast<GLsizei>(m_vertices.size() / 6);
                glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, instanceCount);
            }

            glLineWidth(1.0f);
            glDisable(GL_POLYGON_OFFSET_FILL);
            break;
        }

        // Ripristina lo stato di default
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(0);
    }

    // Metodo per configurare gli attributi delle istanze nel VAO
    virtual void setupInstancedAttributes() {
        if (!m_initialized) return;

        glBindVertexArray(m_vao);

        // Configura gli attributi per la matrice modello (location 3-6)
        // Ogni riga della matrice mat4 occupa un attributo separato
        GLsizei vec4Size = sizeof(glm::vec4);

        // Assumiamo che il VBO delle istanze sia già stato bindato dal manager
        for (int i = 0; i < 4; ++i) {
            glEnableVertexAttribArray(3 + i);
            glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                reinterpret_cast<const void*>(static_cast<uintptr_t>(i * vec4Size)));
            glVertexAttribDivisor(3 + i, 1);  // Avanza ogni istanza
        }

        glBindVertexArray(0);
    }

    // Getter per accedere ai buffer OpenGL
    GLuint getVAO() const { return m_vao; }
    GLuint getVBO() const { return m_vbo; }
    GLuint getEBO() const { return m_ebo; }

    // Getter per i dati geometrici
    const std::vector<float>& getVertices() const { return m_vertices; }
    const std::vector<unsigned int>& getIndices() const { return m_indices; }

    // Metodo per verificare se il modello supporta il rendering istanziato
    virtual bool supportsInstancing() const { return true; }

    RenderMode renderMode = RenderMode::SOLID;
    glm::vec3 color = glm::vec3(0.8f, 0.8f, 0.8f); // Colore di default grigio chiaro

    // Gestione texture
    void setTexture(GLuint texID) { textureID = texID; useTexture = true; }
    virtual void setName(const std::string& name) { m_name = name; }
    void clearTexture() { textureID = 0; useTexture = false; }
    GLuint getTextureID() const { return textureID; }
    bool hasTexture() const { return useTexture && textureID != 0; }

    // Gestione tipo di texture
    void setTextureType(TextureType type) { textureType = type; }
    TextureType getTextureType() const { return textureType; }

    void setColor(const glm::vec3& newColor) { color = newColor; }

protected:
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    std::vector<float> m_vertices;
    std::vector<unsigned int> m_indices;

    std::string m_name;
    bool m_initialized;

    void updateColors(float currentTime);
    RenderMode m_renderMode = RenderMode::SOLID;

    // Metodo helper per configurare i buffer base del modello
	virtual void setupBuffers();

    // Metodo helper per pulire i buffer
    virtual void cleanupBuffers() {
        if (m_ebo != 0) {
            glDeleteBuffers(1, &m_ebo);
            m_ebo = 0;
        }

        if (m_vbo != 0) {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }

        if (m_vao != 0) {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }
    }

    GLuint textureID = 0;
    bool useTexture = false;
    TextureType textureType = TextureType::NONE;
    static GLuint defaultTextureID;
    static void initializeDefaultTexture();
	GLsizei m_indexCount = 0;
};