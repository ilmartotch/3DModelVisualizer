#pragma once
#include "SceneObject.h"
#include "ModelManager.h"
#include "Shaders.h" // Per SetUniformMat4
#include <vector>
#include <memory>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

class SceneManager {
private:
    std::vector<std::shared_ptr<SceneObject>> objects;
    std::shared_ptr<SceneObject> selectedObject = nullptr;
    ModelManager& modelManager;
    unsigned int nextId = 1;

public:
    SceneManager(ModelManager& manager) : modelManager(manager) {}

    // Metodi principali
    std::shared_ptr<SceneObject> addObject(const std::string& modelName,
        const glm::vec3& pos = glm::vec3(0.0f, 0.5f, 0.0f));
    bool removeObject(unsigned int id);
    void selectObject(unsigned int id);
    void deselectAll();

    // Rendering
    void renderAll(GLuint shader, const glm::mat4& view, const glm::mat4& projection,
        Model::RenderMode renderMode);
    void renderForPicking(GLuint pickingShader, const glm::mat4& view, const glm::mat4& projection);

    // Getter
    const std::vector<std::shared_ptr<SceneObject>>& getObjects() const { return objects; }
    std::shared_ptr<SceneObject> getSelectedObject() const { return selectedObject; }
    size_t getObjectCount() const { return objects.size(); }
    bool hasObjects() const { return !objects.empty(); }

    // Utility per picking
    glm::vec3 idToColor(unsigned int id);
    unsigned int colorToId(const glm::vec3& color);
    std::shared_ptr<SceneObject> getObjectById(unsigned int id);

    // Metodi di aggiornamento oggetti
    void updateObjectPosition(unsigned int id, const glm::vec3& position);
    void updateObjectRotation(unsigned int id, const glm::vec3& rotation);
    void updateObjectScale(unsigned int id, const glm::vec3& scale);

private:
    void renderObject(std::shared_ptr<SceneObject> obj, GLuint shader,
        const glm::mat4& view, const glm::mat4& projection,
        Model::RenderMode renderMode);
    void renderObjectForPicking(std::shared_ptr<SceneObject> obj, GLuint pickingShader,
        const glm::mat4& view, const glm::mat4& projection);
    void renderOutline(std::shared_ptr<SceneObject> obj, GLuint shader, const glm::mat4& modelMatrix);
    void applyRenderMode(Model::RenderMode mode);
};

//la classe SceneManager è fondamentale per la gestione degli oggetti nella scena, il renderig e la selezione
// e fornisce metodi per aggiungere, rimuovere e selezionare oggetti, oltre a gestire il rendering
// sviluppo dei metodi all'intenro di SceneManager.cpp