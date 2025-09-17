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

    // Memorizzare la posizione iniziale
    glm::vec3 m_initialPosition = glm::vec3(0.0f, 0.0f, 0.0f);

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

    // Salva la posizione di spawn come posizione iniziale
    void setInitialPosition(const glm::vec3& pos) { m_initialPosition = pos; }
    
    // Ottieni la posizione iniziale salvata
    const glm::vec3& getInitialPosition() const { return m_initialPosition; }
    
	// Ripristina la posizione iniziale
    void resetToInitialPosition() { m_position = m_initialPosition; }

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

    // Modifica nome dei modelli
	void setName(const std::string& newName) { m_name = newName; }

    const std::string& getModelType() const { return m_name; }

};