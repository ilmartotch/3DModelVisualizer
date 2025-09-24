// Source/SceneManager.cpp
#include "../Include/SceneManager.h"
#include <iostream>

std::shared_ptr<SceneObject> SceneManager::addObject(const std::string& modelName, const glm::vec3& pos) {
    auto model = modelManager.getModel(modelName);
    if (!model) {
        std::cerr << "Modello non trovato: " << modelName << std::endl;
        return nullptr;
    }

    auto obj = std::make_shared<SceneObject>(nextId++, model, modelName + "_" + std::to_string(nextId - 1));
    obj->setPosition(pos);
    obj->setInitialPosition(pos); // Salva la posizione iniziale
    objects.push_back(obj);
    return obj;
}

// Implementazione del metodo removeObject
bool SceneManager::removeObject(unsigned int id) {
    // Trova l'oggetto da rimuovere
    auto it = std::find_if(objects.begin(), objects.end(),
                          [id](const std::shared_ptr<SceneObject>& obj) {
                              return obj->getId() == id;
                          });
    
    if (it == objects.end()) {
        return false;  // Oggetto non trovato
    }
    
    // Se stiamo rimuovendo l'oggetto selezionato, deseleziona tutto
    if ((*it)->getSelected()) {
        selectedObjectId = 0;
    }
    
    // Rimuovi l'oggetto dalla lista
    objects.erase(it);
    
    return true;
}

void SceneManager::selectObject(unsigned int id) {
    deselectAll();

    auto it = std::find_if(objects.begin(), objects.end(),
        [id](const auto& obj) { return obj->getId() == id; });

    if (it != objects.end()) {
        selectedObject = *it;
        selectedObject->setSelected(true);
    }
}

void SceneManager::deselectAll() {
    if (selectedObject) {
        selectedObject->setSelected(false);
        selectedObject = nullptr;
    }
}

void SceneManager::renderAll(GLuint shader, const glm::mat4& view, const glm::mat4& projection,
    Model::RenderMode renderMode) {
    for (auto& obj : objects) {
        renderObject(obj, shader, view, projection, renderMode);
    }
}

void SceneManager::renderForPicking(GLuint pickingShader, const glm::mat4& view, const glm::mat4& projection) {
    for (auto& obj : objects) {
        renderObjectForPicking(obj, pickingShader, view, projection);
    }
}

void SceneManager::renderObject(std::shared_ptr<SceneObject> obj, GLuint shader,
    const glm::mat4& view, const glm::mat4& projection,
    Model::RenderMode renderMode) {
    if (!obj->getModel()) return;

    glUseProgram(shader);

    glm::mat4 modelMatrix = obj->getModelMatrix();
    SetUniformMat4(shader, "model", modelMatrix);
    SetUniformMat4(shader, "view", view);
    SetUniformMat4(shader, "projection", projection);

    GLint colorLocation = glGetUniformLocation(shader, "objectColor");
    if (colorLocation != -1) {
        if (obj->getSelected()) {
            glUniform3f(colorLocation, 1.0f, 0.8f, 0.2f); // Arancione per selezione
        }
        else {
            glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f); // Bianco normale
        }
    }

    applyRenderMode(renderMode);
    obj->getModel()->render();

    if (obj->getSelected()) {
        renderOutline(obj, shader, modelMatrix);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void SceneManager::renderObjectForPicking(std::shared_ptr<SceneObject> obj, GLuint pickingShader,
    const glm::mat4& view, const glm::mat4& projection) {
    if (!obj->getModel()) return;

    glUseProgram(pickingShader);

    glm::mat4 modelMatrix = obj->getModelMatrix();
    SetUniformMat4(pickingShader, "model", modelMatrix);
    SetUniformMat4(pickingShader, "view", view);
    SetUniformMat4(pickingShader, "projection", projection);

    glm::vec3 idColor = idToColor(obj->getId());
    GLint idColorLocation = glGetUniformLocation(pickingShader, "idColor");
    if (idColorLocation != -1) {
        glUniform3fv(idColorLocation, 1, glm::value_ptr(idColor));
    }

    obj->getModel()->render();
}

void SceneManager::renderOutline(std::shared_ptr<SceneObject> obj, GLuint shader, const glm::mat4& modelMatrix) {
    GLint colorLocation = glGetUniformLocation(shader, "objectColor");

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glm::mat4 outlineMatrix = glm::scale(modelMatrix, glm::vec3(1.05f));

    SetUniformMat4(shader, "model", outlineMatrix);
    if (colorLocation != -1)
        glUniform3f(colorLocation, 1.0f, 1.0f, 0.0f); // Giallo

    obj->getModel()->render();
}

void SceneManager::applyRenderMode(Model::RenderMode mode) {
    switch (mode) {
    case Model::RenderMode::WIREFRAME:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case Model::RenderMode::SOLID_WITH_WIREFRAME:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    default:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    }
}

glm::vec3 SceneManager::idToColor(unsigned int id) {
    float r = ((id >> 16) & 0xFF) / 255.0f;
    float g = ((id >> 8) & 0xFF) / 255.0f;
    float b = (id & 0xFF) / 255.0f;
    return glm::vec3(r, g, b);
}

unsigned int SceneManager::colorToId(const glm::vec3& color) {
    unsigned int r = static_cast<unsigned int>(color.r * 255.0f + 0.5f);
    unsigned int g = static_cast<unsigned int>(color.g * 255.0f + 0.5f);
    unsigned int b = static_cast<unsigned int>(color.b * 255.0f + 0.5f);
    return (r << 16) | (g << 8) | b;
}

std::shared_ptr<SceneObject> SceneManager::getObjectById(unsigned int id) {
    auto it = std::find_if(objects.begin(), objects.end(),
        [id](const auto& obj) { return obj->getId() == id; });
    return (it != objects.end()) ? *it : nullptr;
}

void SceneManager::updateObjectPosition(unsigned int id, const glm::vec3& position) {
    auto obj = getObjectById(id);
    if (obj) obj->setPosition(position);
}

void SceneManager::updateObjectRotation(unsigned int id, const glm::quat& rotation) {
    auto obj = getObjectById(id);
    if (obj) {
        obj->setRotation(rotation);
    }
}

void SceneManager::updateObjectScale(unsigned int id, const glm::vec3& scale) {
    auto obj = getObjectById(id);
    if (obj) obj->setScale(scale);
}

bool SceneManager::isPositionOccupied(const glm::vec3& position, float radius) const {
    for (const auto& obj : objects) {
        // Controllo della distanza sul piano XZ (ignora Y perché gli oggetti sono appoggiati sul piano)
        float distance = glm::distance(
            glm::vec2(obj->getPosition().x, obj->getPosition().z),
            glm::vec2(position.x, position.z)
        );
        
        if (distance < radius * 2.0f) {
            return true; // Posizione occupata
        }
    }
    return false; // Posizione libera
}

glm::vec3 SceneManager::findValidSpawnPosition(const glm::vec3& cameraPos, const glm::vec3& cameraTarget, 
                                             float yOffset, float padding) const {
    // Calcola la direzione di vista
    glm::vec3 viewDir = glm::normalize(cameraTarget - cameraPos);
    
    // Punto di intersezione con il piano y=0
    glm::vec3 desiredPos;
    
    if (viewDir.y < -0.01f) { // Assicurati che la camera stia guardando verso il basso
        // Calcola l'intersezione con il piano y=0
        float t = -cameraPos.y / viewDir.y;
        desiredPos = cameraPos + t * viewDir;
        desiredPos.y = yOffset; // Imposta l'altezza desiderata
    } else {
        // Fallback se la telecamera è parallela o guarda verso l'alto
        desiredPos = glm::vec3(cameraTarget.x, yOffset, cameraTarget.z);
    }
    
    // Se la posizione desiderata è libera, usala direttamente
    if (!isPositionOccupied(desiredPos, padding)) {
        return desiredPos;
    }
    
    // Altrimenti, cerca in un pattern a spirale attorno al punto desiderato
    const float spiralGrowth = 0.8f; // Fattore di crescita della spirale (più piccolo = più denso)
    const int maxAttempts = 30;      // Numero massimo di tentativi
    
    // Angolo iniziale casuale per evitare pattern prevedibili
    float angle = static_cast<float>(rand()) / RAND_MAX * glm::two_pi<float>();
    float radius = padding * 2.0f;
    
    for (int i = 0; i < maxAttempts; i++) {
        // Calcola posizione sulla spirale
        float x = desiredPos.x + radius * cosf(angle);
        float z = desiredPos.z + radius * sinf(angle);
        glm::vec3 testPos = glm::vec3(x, yOffset, z);
        
        if (!isPositionOccupied(testPos, padding)) {
            return testPos;
        }
        
        // Incrementa per la prossima posizione sulla spirale
        angle += glm::pi<float>() * 0.5f; // 90 gradi
        radius += padding * spiralGrowth;
    }
    
    // Se non troviamo uno spazio libero dopo tutti i tentativi, ritorna una posizione distante
    return glm::vec3(desiredPos.x + radius * 2.0f, yOffset, desiredPos.z);
}

// Riposiziona alla posizione iniziale
void SceneManager::resetObjectToInitialPosition(unsigned int id) {
    auto obj = getObjectById(id);
    if (obj) {
        obj->resetToInitialPosition();
    }
}

// Rinomina l'oggetto
void SceneManager::renameObject(unsigned int id, const std::string& newName) {
    auto obj = getObjectById(id);
    if (obj) {
        obj->setName(newName);
	}
}

// Implementazione del metodo di duplicazione
std::shared_ptr<SceneObject> SceneManager::duplicateObject(unsigned int sourceId, const glm::vec3& newPosition) {
    // Trova l'oggetto da duplicare
    std::shared_ptr<SceneObject> sourceObj = nullptr;
    for (const auto& obj : objects) {
        if (obj->getId() == sourceId) {
            sourceObj = obj;
            break;
        }
    }
    
    if (!sourceObj) {
        return nullptr;
    }
    
    // Ottieni il modello originale
    std::string modelType = sourceObj->getModelType();
    
    // Crea un nuovo oggetto dello stesso tipo
    auto newObj = addObject(modelType, newPosition);
    if (!newObj) {
        return nullptr;
    }
    
    // Copia le proprietà (eccetto posizione che è già impostata)
    newObj->setScale(sourceObj->getScale());
    newObj->setRotation(sourceObj->getRotation());
    
    // Genera un nome per la copia
    std::string newName = sourceObj->getName() + " (Copy)";
    newObj->setName(newName);
    
    return newObj;
}