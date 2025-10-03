#pragma once

#include "../src/Include/SceneObject.h"
#include "../src/Include/ModelManager.h"
#include "../src/Include/Shaders.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

class ModelManager;

class SceneManager {
private:
    std::vector<std::shared_ptr<SceneObject>> objects;
    std::shared_ptr<SceneObject> selectedObject = nullptr;
    ModelManager& modelManager;
    unsigned int nextId = 1;
    unsigned int selectedObjectId = 0;

public:
    SceneManager(ModelManager& manager) : modelManager(manager) {}

    // Metodi principali
    std::shared_ptr<SceneObject> addObject(const std::string& modelName,
        const glm::vec3& pos = glm::vec3(0.0f, 0.5f, 0.0f));
    bool removeObject(unsigned int id);
    void selectObject(unsigned int id);
    void deselectAll();
    void resetObjectToInitialPosition(unsigned int id);
    std::shared_ptr<SceneObject> duplicateObject(unsigned int sourceId, const glm::vec3& newPosition);

    // Rendering
    void renderAll(GLuint shader, const glm::mat4& view, const glm::mat4& projection,
        Model::RenderMode renderMode);
    void renderForPicking(GLuint pickingShader, const glm::mat4& view, const glm::mat4& projection);

    // Getter
    const std::vector<std::shared_ptr<SceneObject>>& getObjects() const { return objects; }
    std::shared_ptr<SceneObject> getSelectedObject() const { return selectedObject; }
    size_t getObjectCount() const { return objects.size(); }
    bool hasObjects() const { return !objects.empty(); }

    unsigned int getSelectedObjectId() const {
        return selectedObject ? selectedObject->getId() : 0;
    }

    // Utility per picking
    glm::vec3 idToColor(unsigned int id);
    unsigned int colorToId(const glm::vec3& color);
    std::shared_ptr<SceneObject> getObjectById(unsigned int id);

    // Metodi di aggiornamento oggetti
    void updateObjectPosition(unsigned int id, const glm::vec3& position);
    void updateObjectRotation(unsigned int id, const glm::quat& rotation);
    void updateObjectScale(unsigned int id, const glm::vec3& scale);

    // Verifica se una posizione è occupata
    bool isPositionOccupied(const glm::vec3& position, float radius = 1.0f) const;

    // Calcola una posizione di spawn valida nel campo visivo della telecamera
    glm::vec3 findValidSpawnPosition(const glm::vec3& cameraPos, const glm::vec3& cameraTarget,
                                    float yOffset = 0.5f, float padding = 1.0f) const;

    void renameObject(unsigned int id, const std::string& newName);

private:
    void renderObject(std::shared_ptr<SceneObject> obj, GLuint shader,
        const glm::mat4& view, const glm::mat4& projection,
        Model::RenderMode renderMode);
    void renderObjectForPicking(std::shared_ptr<SceneObject> obj, GLuint pickingShader,
        const glm::mat4& view, const glm::mat4& projection);
    void renderOutline(std::shared_ptr<SceneObject> obj, GLuint shader, const glm::mat4& modelMatrix);
    void applyRenderMode(Model::RenderMode mode);
};