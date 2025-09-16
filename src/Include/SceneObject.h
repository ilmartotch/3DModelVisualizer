#pragma once

#include "Model.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <memory>

class SceneObject {
private:
    unsigned int m_id;  // ID univoco dell'oggetto
    std::shared_ptr<Model> m_model;  // Modello associato
    std::string m_name;  // Nome descrittivo

    // Trasformazioni
    glm::vec3 m_position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 m_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 m_scale = glm::vec3(1.0f, 1.0f, 1.0f);

    bool m_selected = false;  // Flag di selezione

public:
    SceneObject(unsigned int id, std::shared_ptr<Model> model, const std::string& name)
        : m_id(id), m_model(model), m_name(name) {
    }

    // Getters
    unsigned int getId() const { return m_id; }
    std::shared_ptr<Model> getModel() const { return m_model; }
    const std::string& getName() const { return m_name; }

    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec3& getRotation() const { return m_rotation; }
    const glm::vec3& getScale() const { return m_scale; }
    bool getSelected() const { return m_selected; }

    // Setters
    void setPosition(const glm::vec3& pos) { m_position = pos; }
    void setRotation(const glm::vec3& rot) { m_rotation = rot; }
    void setScale(const glm::vec3& scale) { m_scale = scale; }
    void setSelected(bool selected) { m_selected = selected; }

    // Calcola la matrice modello combinata per il rendering
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, m_position);
        model = glm::rotate(model, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, m_scale);
        return model;
    }
};

//rappresentazione di un singolo oggetto nella scena, mantenendo i riferimenti del modello e le trasformazioni
// Include per la gestione di un oggetto nella scena, con ID, modello, nome e trasformazioni
// 
// La funzione getModelMatrix() è particolarmente importante perché calcola la matrice di trasformazione 
// finale utilizzata durante il rendering.