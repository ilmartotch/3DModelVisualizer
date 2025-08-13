#ifndef INSTANCED_MODEL_MANAGER_H
#define INSTANCED_MODEL_MANAGER_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>

#include "Model.h"

// Rappresenta un'istanza di un modello nel mondo 3D
struct ModelInstance {
    unsigned int instanceId;  // ID univoco per l'istanza
    glm::vec3 position;       // Posizione dell'istanza
    glm::vec3 rotation;       // Rotazione dell'istanza in gradi
    glm::vec3 scale;          // Scala dell'istanza
    glm::mat4 modelMatrix;    // Matrice modello calcolata
    glm::vec3 idColor;        // Colore univoco per il picking
    bool isSelected;          // Flag di selezione

	// Costruttore per inizializzare un'istanza
	ModelInstance()
		: instanceId(0), position(0.0f), rotation(0.0f), scale(1.0f),
		modelMatrix(1.0f), idColor(0.0f), isSelected(false) {
		updateModelMatrix();
	}

	// Costruttore con parametri per inizializzare un'istanza
    ModelInstance(unsigned int id, const glm::vec3& pos = glm::vec3(0.0f),
        const glm::vec3& rot = glm::vec3(0.0f),
        const glm::vec3& sc = glm::vec3(1.0f))
        : instanceId(id), position(pos), rotation(rot), scale(sc),
        modelMatrix(1.0f), idColor(0.0f), isSelected(false) {
        updateModelMatrix();
    }

    // Aggiorna la matrice modello in base a posizione, rotazione e scala
    void updateModelMatrix() {
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::scale(modelMatrix, scale);
    }
};

// Gestisce più istanze di un modello 3D
class InstancedModelManager {
public:
    InstancedModelManager();
    ~InstancedModelManager();

    void initialize(std::shared_ptr<Model> baseModel);
    void cleanup();

    // Aggiunge una nuova istanza
    unsigned int addInstance(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));

    // Rimuove un'istanza
    bool removeInstance(unsigned int instanceId);

    // Aggiorna posizione/rotazione/scala di un'istanza
    void updateInstancePosition(unsigned int instanceId, const glm::vec3& position);
    void updateInstanceRotation(unsigned int instanceId, const glm::vec3& rotation);
    void updateInstanceScale(unsigned int instanceId, const glm::vec3& scale);

    // Seleziona un'istanza
    bool selectInstance(unsigned int instanceId);
    void deselectAll();
    ModelInstance* getSelectedInstance();

    // Rendering
    void updateMatrixBuffer();
    void render(GLuint shader);
	void render(GLuint shader, const glm::mat4& view, const glm::mat4& projection);
    void renderForPicking(GLuint pickingShader);
	void renderForPicking(GLuint pickingShader, const glm::mat4& view, const glm::mat4& projection);

    // Picking
    unsigned int getInstanceIdFromColor(const glm::vec3& color);

    // Getter
    ModelInstance* getInstance(unsigned int instanceId);
    size_t getInstanceCount() const { return m_instances.size(); }
    std::shared_ptr<Model> getBaseModel() const { return m_baseModel; }
	bool isInitialized() const { return m_baseModel != nullptr; }

private:
    std::shared_ptr<Model> m_baseModel;                  // Modello base condiviso da tutte le istanze
    std::unordered_map<unsigned int, ModelInstance> m_instances;  // Mappa di istanze per ID
    unsigned int m_nextInstanceId;                        // Contatore per generare ID istanza

    std::vector<glm::mat4> m_instanceMatrices;           // Buffer matrici per instanced rendering
    unsigned int m_instanceVBO;                          // VBO per matrici istanza
    unsigned int m_selectedInstanceId;                   // ID dell'istanza selezionata

    // Genera un colore univoco basato sull'ID
    glm::vec3 generateColorFromId(unsigned int id);

	void handlePickingResult(const glm::vec3 & idColor);
    void renderTraditionlModelOutline(GLuint shader, const glm::mat4& model);
    void rendrSelectedInstanceOutline(GLuint shader);
};

#endif // INSTANCED_MODEL_MANAGER_H