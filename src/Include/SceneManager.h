#pragma once

#include "../src/Include/SceneObject.h"
#include "../src/Include/ModelManager.h"
#include "../src/Include/Shaders.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <unordered_set>

class ModelManager;
class SceneObject;

class SceneManager {
private:
    std::vector<std::shared_ptr<SceneObject>> objects;
    std::shared_ptr<SceneObject> selectedObject = nullptr;
    ModelManager& modelManager;
    unsigned int nextId = 1;
    unsigned int selectedObjectId = 0;
    std::unordered_set<unsigned int> systemIds;

    // Spazio finito (griglia)
    bool finiteSpaceEnabled = false;
    float finiteHalfSize = 5.0f; // Raggio sul piano XZ
    float finiteMargin = 1.0f; // Padding oltre la griglia
public:
    SceneManager(ModelManager& manager) : modelManager(manager) {}

    // Metodi principali
    std::shared_ptr<SceneObject> addObject(const std::string& modelName,
        const glm::vec3& pos = glm::vec3(0.0f, 0.5f, 0.0f));
    std::shared_ptr<SceneObject> addObject(const std::string& modelName,
                                           const glm::vec3& position,
                                           std::optional<unsigned int> forcedId);
    bool removeObject(unsigned int id);
    void selectObject(unsigned int id);
    void deselectAll();
    void resetObjectToInitialPosition(unsigned int id);
    std::shared_ptr<SceneObject> duplicateObject(unsigned int sourceId, const glm::vec3& newPosition);

    // Rendering
    void renderAll(GLuint shader, const glm::mat4& view, const glm::mat4& projection,
        Model::RenderMode renderMode);
    void renderForPicking(GLuint pickingShader, const glm::mat4& view, const glm::mat4& projection);
    void renderForDepth(GLuint depthShader, unsigned int objectIdToExclude);

    // Getter
    const std::vector<std::shared_ptr<SceneObject>>& getObjects() const { return objects; }
    std::shared_ptr<SceneObject> getSelectedObject() const { return selectedObject; }
    size_t getObjectCount() const { return objects.size(); }
    bool hasObjects() const { return !objects.empty(); }
    unsigned int getSelectedObjectId() const { return selectedObject ? selectedObject->getId() : 0; }

    // Utility per picking
    glm::vec3 idToColor(unsigned int id);
    unsigned int colorToId(const glm::vec3& color);
    std::shared_ptr<SceneObject> getObjectById(unsigned int id);

    // Metodi di aggiornamento oggetti
    void updateObjectPosition(unsigned int id, const glm::vec3& position);
    void updateObjectRotation(unsigned int id, const glm::quat& rotation);
    void updateObjectScale(unsigned int id, const glm::vec3& scale);

    // Controllo occupazione
    bool isPositionOccupied(const glm::vec3& position, float radius = 1.0f) const;

    // Posizione di spawn valida
    glm::vec3 findValidSpawnPosition(const glm::vec3& cameraPos, const glm::vec3& cameraTarget,
                                     float yOffset = 0.5f, float padding = 1.0f) const;

    glm::vec3 findValidSpawnPositionFinite(const glm::vec3& camPos, const glm::vec3& target,
                                            float yOffset, float radius, float halfExtent, float boundaryMargin) const;

    void renameObject(unsigned int id, const std::string& newName);

    // Spazio finito
    void setFiniteSpace(bool enabled, float halfSize, float margin);
    bool hasFiniteSpace() const { return finiteSpaceEnabled; }
    bool isWithinBounds(const glm::vec3& p) const;
    glm::vec3 clampToBounds(const glm::vec3& p) const;

    std::shared_ptr<SceneObject> addSystemObject(
        const std::string& modelName,
        const glm::vec3& position,
        unsigned int fixedId,
        const std::string& displayName
    );

    // Marca/Verifica un ID come "di sistema"
    void markSystemId(unsigned int id);
    bool isSystemId(unsigned int id) const;

    // Applica Nome = baseName + "_" + id (per oggetti utente)
    void applyDefaultName(unsigned int id, const std::string& baseName);

private:
    void renderObject(std::shared_ptr<SceneObject> obj, GLuint shader,
        const glm::mat4& view, const glm::mat4& projection,
        Model::RenderMode renderMode);
    void renderObjectForPicking(std::shared_ptr<SceneObject> obj, GLuint pickingShader,
        const glm::mat4& view, const glm::mat4& projection);
    void renderOutline(std::shared_ptr<SceneObject> obj, GLuint shader, const glm::mat4& modelMatrix);
    void applyRenderMode(Model::RenderMode mode);
};