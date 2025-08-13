#include "../Include/SceneManager.h"

void SceneManager::renderObject(std::shared_ptr<SceneObject> obj, GLuint shader,
    const glm::mat4& view, const glm::mat4& projection,
    Model::RenderMode renderMode) {
    if (!obj->model) return;

    glUseProgram(shader);

    // Imposta matrici
    glm::mat4 modelMatrix = obj->getModelMatrix();
    SetUniformMat4(shader, "model", modelMatrix);
    SetUniformMat4(shader, "view", view);
    SetUniformMat4(shader, "projection", projection);

    // Imposta colore
    GLint colorLocation = glGetUniformLocation(shader, "objectColor");
    if (colorLocation != -1) {
        if (obj->isSelected) {
            glUniform3f(colorLocation, 1.0f, 0.8f, 0.2f); // Arancione per selezione
        }
        else {
            glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f); // Bianco normale
        }
    }

    // Applica modalità rendering
    applyRenderMode(renderMode);

    // Renderizza modello
    obj->model->render();

    // Renderizza outline se selezionato
    if (obj->isSelected) {
        renderOutline(obj, shader, modelMatrix);
    }

    // Ripristina stato
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void SceneManager::renderObjectForPicking(std::shared_ptr<SceneObject> obj, GLuint pickingShader,
    const glm::mat4& view, const glm::mat4& projection) {
    if (!obj->model) return;

    glUseProgram(pickingShader);

    glm::mat4 modelMatrix = obj->getModelMatrix();
    SetUniformMat4(pickingShader, "model", modelMatrix);
    SetUniformMat4(pickingShader, "view", view);
    SetUniformMat4(pickingShader, "projection", projection);

    // Converti ID oggetto in colore RGB per picking
    glm::vec3 idColor = idToColor(obj->id);
    GLint idColorLocation = glGetUniformLocation(pickingShader, "idColor");
    if (idColorLocation != -1) {
        glUniform3fv(idColorLocation, 1, glm::value_ptr(idColor));
    }

    obj->model->render();
}

// Funzione helper per convertire ID in colore
glm::vec3 SceneManager::idToColor(unsigned int id) {
    float r = ((id >> 16) & 0xFF) / 255.0f;
    float g = ((id >> 8) & 0xFF) / 255.0f;
    float b = (id & 0xFF) / 255.0f;
    return glm::vec3(r, g, b);
}

unsigned int SceneManager::colorToId(const glm::vec3& color) {
    unsigned int r = (unsigned int)(color.r * 255.0f);
    unsigned int g = (unsigned int)(color.g * 255.0f);
    unsigned int b = (unsigned int)(color.b * 255.0f);
    return (r << 16) | (g << 8) | b;
}