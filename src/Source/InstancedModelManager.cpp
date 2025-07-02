#include "../Include/InstancedModelManager.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

InstancedModelManager::InstancedModelManager()
    : m_nextInstanceId(1), m_instanceVBO(0), m_selectedInstanceId(0) {
}

InstancedModelManager::~InstancedModelManager() {
    cleanup();
}

void InstancedModelManager::initialize(std::shared_ptr<Model> baseModel) {
    m_baseModel = baseModel;

    // Genera il VBO per le matrici di istanza
    glGenBuffers(1, &m_instanceVBO);

    // Configura il VAO del modello base per l'instanced rendering
    glBindVertexArray(m_baseModel->getVAO());

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    // Inizialmente alloca spazio per 100 istanze (potrebbe essere ridimensionato dinamicamente)
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * 100, nullptr, GL_DYNAMIC_DRAW);

    // Configura gli attributi per la matrice modello (richiede 4 attributi, uno per riga)
    GLsizei vec4Size = sizeof(glm::vec4);

    // Layout: Location 3 = prima riga della matrice
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
    glVertexAttribDivisor(3, 1);  // L'attributo avanza ogni istanza

    // Location 4 = seconda riga
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(vec4Size));
    glVertexAttribDivisor(4, 1);

    // Location 5 = terza riga
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * vec4Size));
    glVertexAttribDivisor(5, 1);

    // Location 6 = quarta riga
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * vec4Size));
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}

void InstancedModelManager::cleanup() {
    if (m_instanceVBO != 0) {
        glDeleteBuffers(1, &m_instanceVBO);
        m_instanceVBO = 0;
    }

    m_instances.clear();
    m_instanceMatrices.clear();
}

unsigned int InstancedModelManager::addInstance(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale) {
    unsigned int instanceId = m_nextInstanceId++;

    auto& instance = m_instances[instanceId];
    instance.instanceId = instanceId;
    instance.position = position;
    instance.rotation = rotation;
    instance.scale = scale;
    instance.isSelected = false;

    // Genera un colore univoco per il picking
    instance.idColor = generateColorFromId(instanceId);

    // Aggiorna la matrice modello
    instance.updateModelMatrix();

    // Aggiorna il buffer delle matrici
    updateMatrixBuffer();

    return instanceId;
}

bool InstancedModelManager::removeInstance(unsigned int instanceId) {
    auto it = m_instances.find(instanceId);
    if (it != m_instances.end()) {
        m_instances.erase(it);
        updateMatrixBuffer();

        // Deseleziona se l'istanza rimossa era selezionata
        if (m_selectedInstanceId == instanceId) {
            m_selectedInstanceId = 0;
        }

        return true;
    }

    return false;
}

void InstancedModelManager::updateInstancePosition(unsigned int instanceId, const glm::vec3& position) {
    auto it = m_instances.find(instanceId);
    if (it != m_instances.end()) {
        it->second.position = position;
        it->second.updateModelMatrix();
        updateMatrixBuffer();
    }
}

void InstancedModelManager::updateInstanceRotation(unsigned int instanceId, const glm::vec3& rotation) {
    auto it = m_instances.find(instanceId);
    if (it != m_instances.end()) {
        it->second.rotation = rotation;
        it->second.updateModelMatrix();
        updateMatrixBuffer();
    }
}

void InstancedModelManager::updateInstanceScale(unsigned int instanceId, const glm::vec3& scale) {
    auto it = m_instances.find(instanceId);
    if (it != m_instances.end()) {
        it->second.scale = scale;
        it->second.updateModelMatrix();
        updateMatrixBuffer();
    }
}

bool InstancedModelManager::selectInstance(unsigned int instanceId) {
    // Deseleziona tutte le istanze
    for (auto& pair : m_instances) {
        pair.second.isSelected = false;
    }

    // Seleziona la nuova istanza
    auto it = m_instances.find(instanceId);
    if (it != m_instances.end()) {
        it->second.isSelected = true;
        m_selectedInstanceId = instanceId;
        return true;
    }

    m_selectedInstanceId = 0;
    return false;
}

void InstancedModelManager::deselectAll() {
    for (auto& pair : m_instances) {
        pair.second.isSelected = false;
    }
    m_selectedInstanceId = 0;
}

ModelInstance* InstancedModelManager::getSelectedInstance() {
    if (m_selectedInstanceId == 0) return nullptr;

    auto it = m_instances.find(m_selectedInstanceId);
    if (it != m_instances.end()) {
        return &(it->second);
    }

    return nullptr;
}

void InstancedModelManager::updateMatrixBuffer() {
    m_instanceMatrices.clear();

    // Prepara il buffer di matrici
    for (const auto& pair : m_instances) {
        m_instanceMatrices.push_back(pair.second.modelMatrix);
    }

    // Aggiorna il VBO
    if (!m_instanceMatrices.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);

        // Ridimensiona il buffer se necessario
        size_t requiredSize = sizeof(glm::mat4) * m_instanceMatrices.size();
        GLint bufferSize;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);

        if (requiredSize > static_cast<size_t>(bufferSize)) {
            // Alloca un buffer più grande
            glBufferData(GL_ARRAY_BUFFER, requiredSize, m_instanceMatrices.data(), GL_DYNAMIC_DRAW);
        }
        else {
            // Aggiorna il buffer esistente
            glBufferSubData(GL_ARRAY_BUFFER, 0, requiredSize, m_instanceMatrices.data());
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void InstancedModelManager::render(GLuint shader) {
    if (m_instances.empty() || !m_baseModel) return;

    // Attiva lo shader per il rendering normale
    glUseProgram(shader);

    // Configura rendering per istanze con matrice di modello istanziata
    m_baseModel->renderInstanced(m_instances.size());
}

void InstancedModelManager::renderForPicking(GLuint pickingShader) {
    if (m_instances.empty() || !m_baseModel) return;

    // Attiva lo shader di picking
    glUseProgram(pickingShader);

    // Per ogni istanza, invia il suo colore ID univoco e renderizza
    for (size_t i = 0; i < m_instances.size(); i++) {
        auto it = m_instances.begin();
        std::advance(it, i);

        // Imposta l'ID colore per questa istanza
        glUniform3f(glGetUniformLocation(pickingShader, "idColor"),
            it->second.idColor.r,
            it->second.idColor.g,
            it->second.idColor.b);

        // Imposta la matrice modello per questa istanza
        glUniformMatrix4fv(glGetUniformLocation(pickingShader, "model"),
            1, GL_FALSE,
            glm::value_ptr(it->second.modelMatrix));

        // Renderizza l'istanza
        m_baseModel->render();
    }
}

unsigned int InstancedModelManager::getInstanceIdFromColor(const glm::vec3& color) {
    // Cerchiamo l'istanza con il colore ID corrispondente
    const float threshold = 0.01f;  // Piccola tolleranza per errori di precisione

    for (const auto& pair : m_instances) {
        const auto& idColor = pair.second.idColor;

        if (glm::abs(idColor.r - color.r) < threshold &&
            glm::abs(idColor.g - color.g) < threshold &&
            glm::abs(idColor.b - color.b) < threshold) {
            return pair.first;
        }
    }

    return 0;  // Nessuna istanza trovata
}

ModelInstance* InstancedModelManager::getInstance(unsigned int instanceId) {
    auto it = m_instances.find(instanceId);
    if (it != m_instances.end()) {
        return &(it->second);
    }
    return nullptr;
}

glm::vec3 InstancedModelManager::generateColorFromId(unsigned int id) {
    // Genera un colore RGB univoco basato sull'ID
    // Usiamo la codifica a 24 bit (8 bit per componente)
    float r = ((id & 0xFF0000) >> 16) / 255.0f;
    float g = ((id & 0x00FF00) >> 8) / 255.0f;
    float b = (id & 0x0000FF) / 255.0f;

    return glm::vec3(r, g, b);
}