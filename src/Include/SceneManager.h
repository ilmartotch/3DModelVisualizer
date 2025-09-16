#pragma once

#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include "SceneObject.h"
#include "ModelManager.h"

class SceneManager {
public:
    SceneManager(ModelManager& modelManager);

    std::shared_ptr<SceneObject> addObject(const std::string& modelName, const glm::vec3& position);
    void removeObject(unsigned int objectId);
    void selectObject(unsigned int objectId);
    void deselectAll();
    std::shared_ptr<SceneObject> getSelectedObject() const;

    void updateObjectPosition(unsigned int objectId, const glm::vec3& newPosition);
    void updateObjectRotation(unsigned int objectId, const glm::vec3& newRotation);
    void updateObjectScale(unsigned int objectId, const glm::vec3& newScale);
    void updateObjectName(unsigned int objectId, const std::string& newName);

    void renderAll(GLuint shader, const glm::mat4& view, const glm::mat4& projection, Model::RenderMode renderMode);
    void renderForPicking(GLuint pickingShader, const glm::mat4& view, const glm::mat4& projection);

    const std::vector<std::shared_ptr<SceneObject>>& getObjects() const;
    size_t getObjectCount() const;
    unsigned int colorToId(const glm::vec3& color) const;

private:
    ModelManager& modelManager;
    std::vector<std::shared_ptr<SceneObject>> objects;
    unsigned int nextId;
    unsigned int selectedObjectId;
};