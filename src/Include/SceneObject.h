#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <memory>
#include "Model.h"

class SceneObject {
private:
    unsigned int m_id;
    std::shared_ptr<Model> m_model;
    std::string m_name;

    // Trasformazioni
    glm::vec3 m_position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Modificato: da vec3 a quat
    glm::vec3 m_scale = glm::vec3(1.0f, 1.0f, 1.0f);

    bool m_selected = false;
    glm::vec3 m_initialPosition = glm::vec3(0.0f, 0.0f, 0.0f);

public:
    SceneObject(unsigned int id, std::shared_ptr<Model> model, const std::string& name)
        : m_id(id), m_model(model), m_name(name) {}

    // Getters
    unsigned int getId() const { return m_id; }
    std::shared_ptr<Model> getModel() const { return m_model; }
    const std::string& getName() const { return m_name; }
    const glm::vec3& getPosition() const { return m_position; }
    const glm::quat& getRotation() const { return m_rotation; } // Modificato
    const glm::vec3& getScale() const { return m_scale; }
    bool getSelected() const { return m_selected; }

    // Setters
    void setPosition(const glm::vec3& pos) { m_position = pos; }
    void setRotation(const glm::quat& rot) { m_rotation = rot; } // Modificato
    void setScale(const glm::vec3& scale) { m_scale = scale; }
    void setSelected(bool selected) { m_selected = selected; }
    void setInitialPosition(const glm::vec3& pos) { m_initialPosition = pos; }
    const glm::vec3& getInitialPosition() const { return m_initialPosition; }
    void resetToInitialPosition() { m_position = m_initialPosition; }

    // Calcola la matrice modello
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), m_position);
        model *= glm::mat4_cast(m_rotation); // Usa il quaternione per la rotazione
        model = glm::scale(model, m_scale);
        return model;
    }

    void setName(const std::string& newName) { m_name = newName; }

    std::string getModelType() const { return m_model ? m_model->getName() : ""; }
};