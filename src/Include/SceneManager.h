#pragma once
#include "SceneObject.h"
#include "ModelManager.h"
#include <vector>
#include <memory>


class SceneManager {
private:
    std::vector<std::shared_ptr<SceneObject>> objects;
    std::shared_ptr<SceneObject> selectedObject = nullptr;
    ModelManager& modelManager; // Riferimento al ModelManager esistente
    unsigned int nextId = 1;

public:
    SceneManager(ModelManager& manager) : modelManager(manager) {}

    // Aggiungi oggetto alla scena
    std::shared_ptr<SceneObject> addObject(const std::string& modelName,
        const glm::vec3& pos = glm::vec3(0.0f, 0.5f, 0.0f)) {
        // Ottieni il modello dal ModelManager esistente
        auto model = modelManager.getModel(modelName);
        if (!model) return nullptr;

        auto obj = std::make_shared<SceneObject>(nextId++, model, modelName + "_" + std::to_string(nextId));
        obj->position = pos;
        objects.push_back(obj);
        return obj;
    }

    // Rimuovi oggetto
    bool removeObject(unsigned int id) {
        auto it = std::find_if(objects.begin(), objects.end(),
            [id](const auto& obj) { return obj->id == id; });

        if (it != objects.end()) {
            if (selectedObject && selectedObject->id == id) {
                selectedObject = nullptr;
            }
            objects.erase(it);
            return true;
        }
        return false;
    }

    // Selezione oggetti
    void selectObject(unsigned int id) {
        // Deseleziona oggetto precedente
        if (selectedObject) {
            selectedObject->isSelected = false;
        }

        // Seleziona nuovo oggetto
        auto it = std::find_if(objects.begin(), objects.end(),
            [id](const auto& obj) { return obj->id == id; });

        if (it != objects.end()) {
            selectedObject = *it;
            selectedObject->isSelected = true;
        }
        else {
            selectedObject = nullptr;
        }
    }

    // Rendering
    void renderAll(GLuint shader, const glm::mat4& view, const glm::mat4& projection,
        Model::RenderMode renderMode) {
        for (auto& obj : objects) {
            renderObject(obj, shader, view, projection, renderMode);
        }
    }

    void renderForPicking(GLuint pickingShader, const glm::mat4& view, const glm::mat4& projection) {
        for (auto& obj : objects) {
            renderObjectForPicking(obj, pickingShader, view, projection);
        }
    }

    // Getter
    const std::vector<std::shared_ptr<SceneObject>>& getObjects() const { return objects; }
    std::shared_ptr<SceneObject> getSelectedObject() const { return selectedObject; }

private:
    void renderObject(std::shared_ptr<SceneObject> obj, GLuint shader,
        const glm::mat4& view, const glm::mat4& projection,
        Model::RenderMode renderMode);

    void renderObjectForPicking(std::shared_ptr<SceneObject> obj, GLuint pickingShader,
        const glm::mat4& view, const glm::mat4& projection);
};