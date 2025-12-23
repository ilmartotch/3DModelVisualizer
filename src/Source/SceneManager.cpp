#include "../Include/SceneManager.h"
#include "../src/Include/SceneObject.h"
#include "../src/Include/ModelManager.h"
#include <iostream>
#include <cstdlib>

std::shared_ptr<SceneObject> SceneManager::addObject(const std::string& modelName, const glm::vec3& pos) {
    auto model = modelManager.getModel(modelName);
    if (!model) {
        std::cerr << "Modello non trovato: " << modelName << std::endl;
        return nullptr;
    }

    glm::vec3 finalPos = hasFiniteSpace() ? clampToBounds(pos) : pos;

    auto obj = std::make_shared<SceneObject>(nextId++, model, modelName + "_" + std::to_string(nextId - 1));
    obj->setPosition(finalPos);
    obj->setInitialPosition(finalPos);
    objects.push_back(obj);
    return obj;
}

std::shared_ptr<SceneObject> SceneManager::addObject(const std::string& modelName,
    const glm::vec3& position,
    std::optional<unsigned int> forcedId)
{
    if (forcedId && getObjectById(*forcedId)) {
        return nullptr;
    }

    auto model = modelManager.getModel(modelName);
    if (!model) return nullptr;

    unsigned int id = forcedId ? *forcedId : nextId++;

    auto obj = std::make_shared<SceneObject>(id, model, modelName, position);
    objects.push_back(obj);
    return obj;
}

bool SceneManager::removeObject(unsigned int id) {
    auto it = std::find_if(objects.begin(), objects.end(),
        [id](const std::shared_ptr<SceneObject>& obj) {
            return obj->getId() == id;
        });

    if (it == objects.end()) {
        return false;
    }

    if ((*it)->getSelected()) {
        selectedObjectId = 0;
    }

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

void SceneManager::renderForDepth(GLuint depthShader, unsigned int objectIdToExclude) {
    for (const auto& obj : objects) {
        if (!obj) continue;
        if (obj->getId() == objectIdToExclude) continue;

        glm::mat4 modelMatrix = obj->getModelMatrix();
        SetUniformMat4(depthShader, "model", modelMatrix);

        if (auto model = obj->getModel()) {
            model->render();
        }
    }
}

void SceneManager::renderObject(std::shared_ptr<SceneObject> obj, GLuint shader,
    const glm::mat4& view, const glm::mat4& projection,
    Model::RenderMode renderMode) {
    if (!obj || !obj->getModel()) return;
    glm::mat4 modelMatrix = obj->getModelMatrix();

    glUseProgram(shader);

    SetUniformMat4(shader, "model", modelMatrix);
    SetUniformMat4(shader, "view", view);
    SetUniformMat4(shader, "projection", projection);

    GLint colorLocation = glGetUniformLocation(shader, "objectColor");
    if (colorLocation != -1) {
        if (obj->getSelected()) {
            glUniform3f(colorLocation, 1.0f, 0.8f, 0.2f);
        }
        else {
            glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f);
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
        glUniform3f(colorLocation, 1.0f, 1.0f, 0.0f);

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
    if (obj) {
        glm::vec3 finalPos = hasFiniteSpace() ? clampToBounds(position) : position;
        obj->setPosition(finalPos);
    }
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
        float distance = glm::distance(
            glm::vec2(obj->getPosition().x, obj->getPosition().z),
            glm::vec2(position.x, position.z)
        );

        if (distance < radius * 2.0f) {
            return true;
        }
    }
    return false;
}

glm::vec3 SceneManager::findValidSpawnPosition(const glm::vec3& cameraPos, const glm::vec3& cameraTarget,
    float yOffset, float padding) const {
    glm::vec3 viewDir = glm::normalize(cameraTarget - cameraPos);

    constexpr float kSpawnLift = 0.5f;

    glm::vec3 desiredPos;

    if (viewDir.y < -0.01f) {
        float t = -cameraPos.y / viewDir.y;
        desiredPos = cameraPos + t * viewDir;
        desiredPos.y = yOffset + kSpawnLift;
    }
    else {
        desiredPos = glm::vec3(cameraTarget.x, yOffset + kSpawnLift, cameraTarget.z);
    }

    if (!isPositionOccupied(desiredPos, padding) &&
        (!hasFiniteSpace() || isWithinBounds(desiredPos))) {
        return desiredPos;
    }

    const float spiralGrowth = 0.8f;
    const int maxAttempts = 30;

    float angle = static_cast<float>(rand()) / RAND_MAX * glm::two_pi<float>();
    float radius = padding * 2.0f;

    for (int i = 0; i < maxAttempts; i++) {
        float x = desiredPos.x + radius * cosf(angle);
        float z = desiredPos.z + radius * sinf(angle);
        glm::vec3 testPos(x, yOffset + kSpawnLift, z);

        if (hasFiniteSpace() && !isWithinBounds(testPos)) {}
        else if (!isPositionOccupied(testPos, padding)) {
            return hasFiniteSpace() ? clampToBounds(testPos) : testPos;
        }

        angle += glm::pi<float>() * 0.5f;
        radius += padding * spiralGrowth;
    }

    glm::vec3 fallback(desiredPos.x + radius * 2.0f, yOffset, desiredPos.z);
    return hasFiniteSpace() ? clampToBounds(fallback) : fallback;
}

glm::vec3 SceneManager::findValidSpawnPositionFinite(const glm::vec3& camPos, const glm::vec3& target,
    float yOffset, float radius, float halfExtent,
    float boundaryMargin) const {

    glm::vec3 base = findValidSpawnPosition(camPos, target, yOffset, radius);

    auto clampFn = [&](const glm::vec3& p) -> glm::vec3 {
        float limit = halfExtent + boundaryMargin;
        return glm::vec3(
            std::clamp(p.x, -limit, limit),
            p.y,
            std::clamp(p.z, -limit, limit)
        );
        };

    auto occupied = [&](const glm::vec3& p) -> bool {
        for (const auto& obj : getObjects()) {
            if (isSystemId(obj->getId())) continue;
            glm::vec3 op = obj->getPosition();
            float rOther = radius;
            if (glm::distance(glm::vec2(op.x, op.z), glm::vec2(p.x, p.z)) < (rOther + radius) * 0.95f) {
                return true;
            }
        }
        return false;
        };

    glm::vec3 candidate = clampFn(base);
    if (!occupied(candidate)) {
        return candidate;
    }

    const int maxRings = 10;
    float cell = (radius * 2.0f) + 0.15f;
    for (int ring = 1; ring <= maxRings; ++ring) {
        for (int dx = -ring; dx <= ring; ++dx) {
            glm::vec3 top = candidate + glm::vec3(dx * cell, 0.0f, -ring * cell);
            top = clampFn(top);
            if (!occupied(top)) return top;

            glm::vec3 bottom = candidate + glm::vec3(dx * cell, 0.0f, ring * cell);
            bottom = clampFn(bottom);
            if (!occupied(bottom)) return bottom;
        }
        for (int dz = -ring + 1; dz <= ring - 1; ++dz) {
            glm::vec3 left = candidate + glm::vec3(-ring * cell, 0.0f, dz * cell);
            left = clampFn(left);
            if (!occupied(left)) return left;

            glm::vec3 right = candidate + glm::vec3(ring * cell, 0.0f, dz * cell);
            right = clampFn(right);
            if (!occupied(right)) return right;
        }
    }

    return candidate;
}

void SceneManager::resetObjectToInitialPosition(unsigned int id) {
    auto obj = getObjectById(id);
    if (obj) {
        glm::vec3 p = obj->getInitialPosition();
        obj->setPosition(hasFiniteSpace() ? clampToBounds(p) : p);
    }
}

void SceneManager::renameObject(unsigned int id, const std::string& newName) {
    auto obj = getObjectById(id);
    if (obj) obj->setName(newName);
}

std::shared_ptr<SceneObject> SceneManager::duplicateObject(unsigned int sourceId, const glm::vec3& newPosition) {
    auto sourceObj = getObjectById(sourceId);
    if (!sourceObj) return nullptr;

    std::string modelType = sourceObj->getName();
    auto newObj = addObject(modelType, newPosition);
    if (!newObj) return nullptr;

    newObj->setScale(sourceObj->getScale());
    newObj->setRotation(sourceObj->getRotation());
    newObj->setName(sourceObj->getName() + " (Copy)");
    return newObj;
}

void SceneManager::setFiniteSpace(bool enabled, float halfSize, float margin) {
    finiteSpaceEnabled = enabled;
    finiteHalfSize = halfSize;
    finiteMargin = margin;
}

bool SceneManager::isWithinBounds(const glm::vec3& p) const {
    if (!finiteSpaceEnabled) return true;
    return (p.x >= -finiteHalfSize && p.x <= finiteHalfSize &&
        p.z >= -finiteHalfSize && p.z <= finiteHalfSize);
}

glm::vec3 SceneManager::clampToBounds(const glm::vec3& p) const {
    if (!finiteSpaceEnabled) return p;
    glm::vec3 r = p;
    r.x = std::clamp(r.x, -finiteHalfSize, finiteHalfSize);
    r.z = std::clamp(r.z, -finiteHalfSize, finiteHalfSize);
    return r;
}

void SceneManager::markSystemId(unsigned int id) {
    systemIds.insert(id);
}

bool SceneManager::isSystemId(unsigned int id) const {
    return systemIds.find(id) != systemIds.end();
}

void SceneManager::applyDefaultName(unsigned int id, const std::string& baseName) {
    std::string newName = baseName + "_" + std::to_string(id);
    renameObject(id, newName);
}

std::shared_ptr<SceneObject> SceneManager::addSystemObject(
    const std::string& modelName,
    const glm::vec3& position,
    unsigned int fixedId,
    const std::string& displayName)
{
    auto obj = addObject(modelName, position, fixedId);
    if (obj) {
        markSystemId(fixedId);
        renameObject(fixedId, displayName);
    }
    return obj;
}