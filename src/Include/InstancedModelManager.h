#ifndef INSTANCED_MODEL_MANAGER_H
#define INSTANCED_MODEL_MANAGER_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>

#include "Model.h"

struct ModelInstance {
    unsigned int instanceId;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::mat4 modelMatrix;
    glm::vec3 idColor;
    bool isSelected;

	ModelInstance()
		: instanceId(0), position(0.0f), rotation(0.0f), scale(1.0f),
		modelMatrix(1.0f), idColor(0.0f), isSelected(false) {
		updateModelMatrix();
	}

    ModelInstance(unsigned int id, const glm::vec3& pos = glm::vec3(0.0f),
        const glm::vec3& rot = glm::vec3(0.0f),
        const glm::vec3& sc = glm::vec3(1.0f))
        : instanceId(id), position(pos), rotation(rot), scale(sc),
        modelMatrix(1.0f), idColor(0.0f), isSelected(false) {
        updateModelMatrix();
    }

    void updateModelMatrix() {
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::scale(modelMatrix, scale);
    }
};


class InstancedModelManager {
public:
    InstancedModelManager();
    ~InstancedModelManager();

    void initialize(std::shared_ptr<Model> baseModel);
    void cleanup();

    unsigned int addInstance(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));

    bool removeInstance(unsigned int instanceId);

    void updateInstancePosition(unsigned int instanceId, const glm::vec3& position);
    void updateInstanceRotation(unsigned int instanceId, const glm::vec3& rotation);
    void updateInstanceScale(unsigned int instanceId, const glm::vec3& scale);

    bool selectInstance(unsigned int instanceId);
    void deselectAll();
    ModelInstance* getSelectedInstance();

    void updateMatrixBuffer();
    void render(GLuint shader);
	void render(GLuint shader, const glm::mat4& view, const glm::mat4& projection);
    void renderForPicking(GLuint pickingShader);
	void renderForPicking(GLuint pickingShader, const glm::mat4& view, const glm::mat4& projection);

    unsigned int getInstanceIdFromColor(const glm::vec3& color);

    ModelInstance* getInstance(unsigned int instanceId);
    size_t getInstanceCount() const { return m_instances.size(); }
    std::shared_ptr<Model> getBaseModel() const { return m_baseModel; }
	bool isInitialized() const { return m_baseModel != nullptr; }

private:
    std::shared_ptr<Model> m_baseModel;
    std::unordered_map<unsigned int, ModelInstance> m_instances;
    unsigned int m_nextInstanceId;

    std::vector<glm::mat4> m_instanceMatrices;
    unsigned int m_instanceVBO;
    unsigned int m_selectedInstanceId;

    glm::vec3 generateColorFromId(unsigned int id);

	void handlePickingResult(const glm::vec3 & idColor);
    void renderTraditionlModelOutline(GLuint shader, const glm::mat4& model);
    void rendrSelectedInstanceOutline(GLuint shader);
};

#endif