#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <memory>
#include "Model.h"

class SceneObject {
public:
    SceneObject(unsigned int id, const std::string& name, std::shared_ptr<Model> model,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f))
        : id(id), name(name), model(model), position(position), rotation(rotation), scale(scale), selected(false) {}

    void render(GLuint shader, Model::RenderMode renderMode) {
        if (!model) return;

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::scale(modelMatrix, scale);

        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMatrix[0][0]);

        model->setRenderMode(renderMode);
        model->render();
    }

    void renderForPicking(GLuint pickingShader) {
        if (!model) return;

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::scale(modelMatrix, scale);

        glUniformMatrix4fv(glGetUniformLocation(pickingShader, "model"), 1, GL_FALSE, &modelMatrix[0][0]);

        // Converte l'ID in un colore per il picking
        float r = ((id >> 16) & 0xFF) / 255.0f;
        float g = ((id >> 8) & 0xFF) / 255.0f;
        float b = (id & 0xFF) / 255.0f;
        glUniform3f(glGetUniformLocation(pickingShader, "pickingColor"), r, g, b);

        model->render();
    }

    // Getters
    unsigned int getId() const { return id; }
    const std::string& getName() const { return name; }
    const glm::vec3& getPosition() const { return position; }
    const glm::vec3& getRotation() const { return rotation; }
    const glm::vec3& getScale() const { return scale; }
    bool getSelected() const { return selected; }
    std::shared_ptr<Model> getModel() const { return model; }

    // Setters
    void setName(const std::string& newName) { name = newName; }
    void setPosition(const glm::vec3& newPosition) { position = newPosition; }
    void setRotation(const glm::vec3& newRotation) { rotation = newRotation; }
    void setScale(const glm::vec3& newScale) { scale = newScale; }
    void setSelected(bool isSelected) { selected = isSelected; }

private:
    unsigned int id;
    std::string name;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    bool selected;
    std::shared_ptr<Model> model;
};