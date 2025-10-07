#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <string>
#include "Model.h"

class SceneObject {
public:
    SceneObject(unsigned int id, std::shared_ptr<Model> model, const std::string& name,
                const glm::vec3& position = glm::vec3(0.0f),
                const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                const glm::vec3& scale = glm::vec3(1.0f))
        : m_id(id), m_model(model), m_name(name), m_position(position),
          m_rotation(rotation), m_scale(scale), m_selected(false),
          m_initialPosition(position), useOverrideColor(false), overrideColor(1.0f) {
        updateModelMatrix();
    }

    // Getters
    unsigned int getId() const { return m_id; }
    std::shared_ptr<Model> getModel() const { return m_model; }
    const std::string& getName() const { return m_name; }
    const glm::vec3& getPosition() const { return m_position; }
    const glm::quat& getRotation() const { return m_rotation; }
    const glm::vec3& getScale() const { return m_scale; }
    const glm::mat4& getModelMatrix() const { return m_modelMatrix; }
    bool getSelected() const { return m_selected; }
    const glm::vec3& getInitialPosition() const { return m_initialPosition; }

    // Setters
    void setName(const std::string& name) { m_name = name; }
    void setPosition(const glm::vec3& position) { m_position = position; updateModelMatrix(); }
    void setRotation(const glm::quat& rotation) { m_rotation = rotation; updateModelMatrix(); }
    void setScale(const glm::vec3& scale) { m_scale = scale; updateModelMatrix(); }
    void setSelected(bool selected) { m_selected = selected; }
	void serModel(std::shared_ptr<Model> model) { m_model = model; }

    // Metodi per il colore di override
    void setOverrideColor(const glm::vec4& color) {
        overrideColor = color;
        useOverrideColor = true;
    }

    void clearOverrideColor() {
        useOverrideColor = false;
    }

    bool hasOverrideColor() const {
        return useOverrideColor;
    }

    glm::vec4 getOverrideColor() const {
        return overrideColor;
    }

    // Aggiorna la matrice del modello
    void updateModelMatrix() {
        m_modelMatrix = glm::mat4(1.0f);
        m_modelMatrix = glm::translate(m_modelMatrix, m_position);
        m_modelMatrix *= glm::mat4_cast(m_rotation);
        m_modelMatrix = glm::scale(m_modelMatrix, m_scale);
    }

    void setInitialPosition(const glm::vec3& position) { m_initialPosition = position; }

    void resetToInitialPosition() { setPosition(m_initialPosition); }

private:
    unsigned int m_id;
    std::shared_ptr<Model> m_model;
    std::string m_name;

    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_scale;
    glm::mat4 m_modelMatrix;

    bool m_selected;
    glm::vec3 m_initialPosition;

    // Proprietà per il colore di override
    bool useOverrideColor;
    glm::vec4 overrideColor;
};