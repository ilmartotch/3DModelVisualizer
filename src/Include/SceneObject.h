#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "Model.h"
#include <InstancedModelManager.h>

class SceneObject {
public:
    unsigned int id;
    std::shared_ptr<Model> model;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    bool isSelected = false;
    std::string name;

    SceneObject(unsigned int objId, std::shared_ptr<Model> objModel, const std::string& objName)
        : id(objId), model(objModel), name(objName) {
    }

    glm::mat4 getModelMatrix() const {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::scale(modelMatrix, scale);
        return modelMatrix;
    }
};