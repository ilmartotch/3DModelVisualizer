// Source/SceneManager.cpp
#include "../Include/SceneManager.h"
#include <iostream>

SceneManager::SceneManager(ModelManager& modelManager)
    : modelManager(modelManager), nextId(1), selectedObjectId(0) {}

std::shared_ptr<SceneObject> SceneManager::addObject(const std::string& modelName, const glm::vec3& position) {
    auto model = modelManager.getModel(modelName);
    if (model) {
        std::string objectName = modelName + "_" + std::to_string(nextId);
        auto newObject = std::make_shared<SceneObject>(nextId++, objectName, model, position);
        objects.push_back(newObject);
        return newObject;
    }
    return nullptr;
}

void SceneManager::removeObject(unsigned int objectId) {
    objects.erase(std::remove_if(objects.begin(), objects.end(),
        [objectId](const std::shared_ptr<SceneObject>& obj) {
            return obj->getId() == objectId;
        }), objects.end());

    if (selectedObjectId == objectId) {
        selectedObjectId = 0;
    }
}

void SceneManager::selectObject(unsigned int objectId) {
    deselectAll();
    for (auto& obj : objects) {
        if (obj->getId() == objectId) {
            obj->setSelected(true);
            selectedObjectId = objectId;
            break;
        }
    }
}

void SceneManager::deselectAll() {
    if (selectedObjectId != 0) {
        for (auto& obj : objects) {
            if (obj->getSelected()) {
                obj->setSelected(false);
            }
        }
        selectedObjectId = 0;
    }
}

std::shared_ptr<SceneObject> SceneManager::getSelectedObject() const {
    if (selectedObjectId == 0) return nullptr;
    for (const auto& obj : objects) {
        if (obj->getId() == selectedObjectId) {
            return obj;
        }
    }
    return nullptr;
}

void SceneManager::updateObjectPosition(unsigned int objectId, const glm::vec3& newPosition) {
    for (auto& obj : objects) {
        if (obj->getId() == objectId) {
            obj->setPosition(newPosition);
            break;
        }
    }
}

void SceneManager::updateObjectRotation(unsigned int objectId, const glm::vec3& newRotation) {
    for (auto& obj : objects) {
        if (obj->getId() == objectId) {
            obj->setRotation(newRotation);
            break;
        }
    }
}

void SceneManager::updateObjectScale(unsigned int objectId, const glm::vec3& newScale) {
    for (auto& obj : objects) {
        if (obj->getId() == objectId) {
            obj->setScale(newScale);
            break;
        }
    }
}

void SceneManager::updateObjectName(unsigned int objectId, const std::string& newName) {
    for (auto& obj : objects) {
        if (obj->getId() == objectId) {
            obj->setName(newName);
            break;
        }
    }
}

void SceneManager::renderAll(GLuint shader, const glm::mat4& view, const glm::mat4& projection, Model::RenderMode renderMode) {
    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, &projection[0][0]);

    for (const auto& obj : objects) {
        obj->render(shader, renderMode);
    }
}

void SceneManager::renderForPicking(GLuint pickingShader, const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(pickingShader);
    glUniformMatrix4fv(glGetUniformLocation(pickingShader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pickingShader, "projection"), 1, GL_FALSE, &projection[0][0]);

    for (const auto& obj : objects) {
        obj->renderForPicking(pickingShader);
    }
}

const std::vector<std::shared_ptr<SceneObject>>& SceneManager::getObjects() const {
    return objects;
}

size_t SceneManager::getObjectCount() const {
    return objects.size();
}

unsigned int SceneManager::colorToId(const glm::vec3& color) const {
    if (color.r == 0.0f && color.g == 0.0f && color.b == 0.0f) return 0;
    unsigned int r = static_cast<unsigned int>(color.r * 255.0f);
    unsigned int g = static_cast<unsigned int>(color.g * 255.0f);
    unsigned int b = static_cast<unsigned int>(color.b * 255.0f);
    return (r << 16) | (g << 8) | b;
}