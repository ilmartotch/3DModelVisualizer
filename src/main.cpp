#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Include/Window.h"
#include "Include/Shaders.h"
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <vector>

// Manager per rendering istanziato e picking
#include "Include/InstancedModelManager.h"
#include "Include/PickingBuffer.h"

// ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// Modelli
#include "Include/Model.h"
#include "../Assets/Include/CubeModel.h"
#include "../Assets/Include/SphereModel.h"
#include "../Assets/Include/PyramidModel.h"
#include "Include/Grid.h"
#include "Include/ModelManager.h"
#include "Include/SceneManager.h"

// Forward declarations
void handlePickingResult(const glm::vec3& idColor);
void renderTraditionalModelOutline(GLuint shader, const glm::mat4& model);
void renderSelectedInstanceOutline(GLuint shader);
void processMousePicking(int x, int y);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void updateCameraPosition();
void applyRenderMode(Model::RenderMode mode);
void renderTraditionalModel(GLuint shader, const glm::mat4& view, const glm::mat4& projection);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void initializePickingSystem();
void initializeInstancedRendering();
void renderInstanceControlPanel();
void handlePickingResultUnified(const glm::vec3& idColor);
void processMousePickingUnified(int x, int y);
void renderScene(GLuint shader, const glm::mat4& view, const glm::mat4& projection);
void renderSceneControlPanel();

// Variabili window
int windoWidth = 800;
int windowHeight = 600;
bool windowResized = false;
static bool shoeHelpWindow = false;
const int SIDEBAR_WIDTH = 300;
bool showModelInfo = true;
bool showSelectedModelPanel = false;

// FPS
float fps = 0.0f;
float frameTimeAccumulator = 0.0f;
int frameCount = 0;
glm::vec3 modelScale(1.0f, 1.0f, 1.0f);

// Oggetti e controlli
bool objectMoving = false;
glm::vec3 lastMousePos;
float moveSpeed = 0.1f;
bool objectSelected = false;
glm::vec3 selectedObjectPosition = glm::vec3(0.0f, 0.5f, 0.0f);

// Instanziato
bool useInstancing = false;
InstancedModelManager instancedModelManager;

// Manager per modelli istanziati e picking
PickingBuffer pickingBuffer;
GLuint pickingShader = 0;
bool pickingEnabled = true;

// Tracciamento RenderMode
int currentRenderMode = static_cast<int>(Model::RenderMode::SOLID);

// Input comandi
struct MouseControl {
    bool isPressed = false;
    bool isOrbiting = false;
    float lastX = 0.0f;
    float lastY = 0.0f;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float cameraDistance = 3.0f;
    float cameraHeight = 1.5f;
    glm::vec3 camPos = glm::vec3(0.0f, 1.5f, 3.0f);
    float orbitalAngleX = 0.0f;
    float orbitalAngleY = 0.0f;
};

enum class MoveAxis {
    NONE,
    X_AXIS,
    Y_AXIS,
    Z_AXIS
};

MoveAxis currentMoveAxis = MoveAxis::NONE;

MouseControl mouseControl;

// ModelManager per la gestione dei modelli
ModelManager modelManager;

// SceneManager per la gestione della scena
SceneManager sceneManager(modelManager);

// Ridimensionamento della finestra
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (width > 0 && height > 0) {
        windoWidth = width;
        windowHeight = height;
        windowResized = true;

        glViewport(0, 0, width, height);

        // Ridimensiona anche il buffer di picking quando la finestra cambia dimensione
        if (pickingBuffer.isInitialized()) {
            pickingBuffer.resize(width, height);
        }
    }
}

// Calcolo della nuova posizione della camera
void updateCameraPosition() {
    float radXZ = mouseControl.cameraDistance * cosf(glm::radians(mouseControl.orbitalAngleY));
    float camX = radXZ * sinf(glm::radians(mouseControl.orbitalAngleX));
    float camY = mouseControl.cameraDistance * sinf(glm::radians(mouseControl.orbitalAngleY));
    float camZ = radXZ * cosf(glm::radians(mouseControl.orbitalAngleX));

    mouseControl.camPos = glm::vec3(camX, camY + mouseControl.cameraHeight / 2, camZ);
}

// Funzioni helper per il rendering
void applyRenderMode(Model::RenderMode mode) {
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

void renderTraditionalModel(GLuint shader, const glm::mat4& view, const glm::mat4& projection) {
    if (!modelManager.hasActiveModel()) return;

    modelManager.updateActiveModel(glfwGetTime());

    glUseProgram(shader);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, selectedObjectPosition);
    model = glm::rotate(model, glm::radians(mouseControl.rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(mouseControl.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, modelScale);

    SetUniformMat4(shader, "model", model);
    SetUniformMat4(shader, "view", view);
    SetUniformMat4(shader, "projection", projection);

    GLint colorLocation = glGetUniformLocation(shader, "objectColor");
    if (colorLocation != -1)
        glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f);

    // Applica render mode solo per questo rendering
    applyRenderMode(static_cast<Model::RenderMode>(currentRenderMode));

    modelManager.renderActiveModel();

    // Ripristina stato
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (objectSelected && !useInstancing) {
        renderTraditionalModelOutline(shader, model);
    }
}

void renderTraditionalModelOutline(GLuint shader, const glm::mat4& model) {
    GLint colorLocation = glGetUniformLocation(shader, "objectColor");

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glm::mat4 outlineModel = model;
    outlineModel = glm::scale(outlineModel, glm::vec3(1.05f));

    SetUniformMat4(shader, "model", outlineModel);
    if (colorLocation != -1)
        glUniform3f(colorLocation, 1.0f, 1.0f, 0.0f); // Giallo

    modelManager.renderActiveModel();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void renderSelectedInstanceOutline(GLuint shader) {
    ModelInstance* selected = instancedModelManager.getSelectedInstance();
    if (!selected) return;

    GLint colorLocation = glGetUniformLocation(shader, "objectColor");

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glm::mat4 outlineModel = glm::mat4(1.0f);
    outlineModel = glm::translate(outlineModel, selected->position);
    outlineModel = glm::rotate(outlineModel, glm::radians(selected->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    outlineModel = glm::rotate(outlineModel, glm::radians(selected->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    outlineModel = glm::rotate(outlineModel, glm::radians(selected->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    outlineModel = glm::scale(outlineModel, selected->scale * 1.05f);

    SetUniformMat4(shader, "model", outlineModel);
    if (colorLocation != -1)
        glUniform3f(colorLocation, 1.0f, 1.0f, 0.0f);  // Giallo

    instancedModelManager.getBaseModel()->render();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// Gestione del picking con ID buffer unificata
void processMousePicking(int x, int y) {
    if (!pickingEnabled || !pickingBuffer.isInitialized()) return;

    // Attiva il framebuffer per il picking
    pickingBuffer.bind();
    pickingBuffer.clear();

    // Configura le matrici di vista e proiezione per il rendering di picking
    float aspect = static_cast<float>(windoWidth) / static_cast<float>(windowHeight);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(
        mouseControl.camPos,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glUseProgram(pickingShader);
    SetUniformMat4(pickingShader, "view", view);
    SetUniformMat4(pickingShader, "projection", projection);

    // Renderizza PRIMA le istanze (se abilitate)
    if (useInstancing && instancedModelManager.isInitialized()) {
        instancedModelManager.renderForPicking(pickingShader, view, projection);
    }

    // POI renderizza i modelli tradizionali con ID diversi
    if (modelManager.hasActiveModel()) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, selectedObjectPosition);
        model = glm::rotate(model, glm::radians(mouseControl.rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(mouseControl.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, modelScale);

        SetUniformMat4(pickingShader, "model", model);

        // Usa un ID colore diverso per il modello tradizionale
        glm::vec3 traditionalIdColor(0.0f, 1.0f, 0.0f); // Verde per distinguere
        glUniform3fv(glGetUniformLocation(pickingShader, "idColor"), 1, glm::value_ptr(traditionalIdColor));

        modelManager.renderActiveModel();
    }

    // Leggi l'ID colore alla posizione del mouse
    glm::vec3 idColor = pickingBuffer.readPixel(x, y);

    // Disattiva il framebuffer di picking
    pickingBuffer.unbind();

    // Ripristina la viewport originale
    glViewport(0, 0, windoWidth, windowHeight);

    // Gestisci selezione considerando entrambi i sistemi
    handlePickingResult(idColor);
}

void handlePickingResult(const glm::vec3& idColor) {
    // Prima controlla se è un modello tradizionale (verde)
    if (abs(idColor.y - 1.0f) < 0.01f && idColor.x < 0.01f && idColor.z < 0.01f) {
        // Modello tradizionale selezionato
        objectSelected = true;
        showSelectedModelPanel = true;
        // Deseleziona eventuali istanze
        instancedModelManager.deselectAll();
        return;
    }

    // Poi controlla se è un'istanza
    if (useInstancing) {
        unsigned int instanceId = instancedModelManager.getInstanceIdFromColor(idColor);
        if (instanceId > 0) {
            // Un'istanza è stata selezionata
            instancedModelManager.selectInstance(instanceId);
            objectSelected = true;
            showSelectedModelPanel = true;

            // Ottieni i dati dell'istanza selezionata
            ModelInstance* instance = instancedModelManager.getSelectedInstance();
            if (instance) {
                selectedObjectPosition = instance->position;
                modelScale = instance->scale;
                mouseControl.rotationX = instance->rotation.x;
                mouseControl.rotationY = instance->rotation.y;
            }
        }
        else {
            // Nessuna istanza selezionata
            instancedModelManager.deselectAll();
            objectSelected = false;
            showSelectedModelPanel = false;
        }
    }
    else {
        // Logica di selezione per il rendering normale
        if (idColor.x > 0.0f || idColor.y > 0.0f || idColor.z > 0.0f) {
            objectSelected = true;
            showSelectedModelPanel = true;
        }
        else {
            objectSelected = false;
            showSelectedModelPanel = false;
        }
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Prima verifichiamo se il mouse è sopra ImGui
            if (ImGui::GetIO().WantCaptureMouse) {
                return; // Non gestire il click se ImGui lo vuole catturare
            }

            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            // Se il picking è abilitato, gestiamo la selezione tramite il buffer di ID
            if (pickingEnabled) {
                // Usiamo il nuovo metodo unificato di picking
                processMousePickingUnified(static_cast<int>(xpos), static_cast<int>(ypos));
            }

            // Controlla se ALT è premuto per modalità orbita
            if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
                mouseControl.isOrbiting = true;
                mouseControl.isPressed = false; // Disabilita la rotazione normale
            }
            else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && objectSelected) {
                objectMoving = true;

                if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
                    currentMoveAxis = MoveAxis::X_AXIS;
                }
                else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    currentMoveAxis = MoveAxis::Y_AXIS;
                }
                else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                    currentMoveAxis = MoveAxis::Z_AXIS;
                }
                else {
                    currentMoveAxis = MoveAxis::NONE; // Nessun asse specificato
                }

                lastMousePos = glm::vec3((float)xpos, (float)ypos, 0.0f);
            }
            else {
                if (!pickingEnabled && objectSelected) {
                    objectSelected = true;
                }
                mouseControl.isOrbiting = false;
                mouseControl.isPressed = true;
            }

            mouseControl.lastX = (float)xpos;
            mouseControl.lastY = (float)ypos;
        }
        else if (action == GLFW_RELEASE) {
            mouseControl.isPressed = false;
            mouseControl.isOrbiting = false;
            objectMoving = false;
            currentMoveAxis = MoveAxis::NONE;
        }
    }
}

// Callback movimento del mouse
void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    float xoffset = (float)xpos - mouseControl.lastX;
    float yoffset = mouseControl.lastY - (float)ypos;

    mouseControl.lastX = (float)xpos;
    mouseControl.lastY = (float)ypos;

    const float sensitivity = 0.3f;

    if (objectMoving) {
        glm::vec3 currentMousePos = glm::vec3((float)xpos, (float)ypos, 0.0f);
        glm::vec3 mouseDelta = currentMousePos - lastMousePos;

        // Aggiorna la posizione dell'oggetto selezionato
        switch (currentMoveAxis) {
        case MoveAxis::X_AXIS:
            selectedObjectPosition.x += mouseDelta.x * moveSpeed;
            break;
        case MoveAxis::Y_AXIS:
            selectedObjectPosition.y -= mouseDelta.y * moveSpeed;
            break;
        case MoveAxis::Z_AXIS:
            selectedObjectPosition.z += mouseDelta.y * moveSpeed;
            break;
        case MoveAxis::NONE:
            selectedObjectPosition.x += mouseDelta.x * moveSpeed;
            selectedObjectPosition.y -= mouseDelta.y * moveSpeed;
            break;
        }

        lastMousePos = currentMousePos;

        // Aggiorna la posizione nell'oggetto selezionato nel SceneManager
        auto selectedObj = sceneManager.getSelectedObject();
        if (selectedObj) {
            sceneManager.updateObjectPosition(selectedObj->getId(), selectedObjectPosition);
        }
        // Supporto legacy per istanze
        else if (useInstancing && objectSelected) {
            ModelInstance* instance = instancedModelManager.getSelectedInstance();
            if (instance) {
                instancedModelManager.updateInstancePosition(instance->instanceId, selectedObjectPosition);
            }
        }
    }
    else if (mouseControl.isPressed) {
        mouseControl.rotationX += yoffset * sensitivity;
        mouseControl.rotationY += xoffset * sensitivity;

        mouseControl.rotationX = fmodf(mouseControl.rotationX, 360.0f);
        mouseControl.rotationY = fmodf(mouseControl.rotationY, 360.0f);

        // Aggiorna la rotazione dell'oggetto selezionato nel SceneManager
        auto selectedObj = sceneManager.getSelectedObject();
        if (selectedObj) {
            sceneManager.updateObjectRotation(selectedObj->getId(),
                glm::vec3(mouseControl.rotationX, mouseControl.rotationY, 0.0f));
        }
        // Supporto legacy per istanze
        else if (useInstancing && objectSelected) {
            ModelInstance* instance = instancedModelManager.getSelectedInstance();
            if (instance) {
                instancedModelManager.updateInstanceRotation(instance->instanceId,
                    glm::vec3(mouseControl.rotationX, mouseControl.rotationY, 0.0f));
            }
        }
    }
    else if (mouseControl.isOrbiting) {
        mouseControl.orbitalAngleX += xoffset * sensitivity;
        mouseControl.orbitalAngleY += yoffset * sensitivity;
        if (mouseControl.orbitalAngleY > 85.0f) mouseControl.orbitalAngleY = 85.0f;
        if (mouseControl.orbitalAngleY < -85.0f) mouseControl.orbitalAngleY = -85.0f;

        updateCameraPosition();
    }
}

// Callback per lo zoom con la rotellina del mouse
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    float zoomSpeed = 0.5f;
    mouseControl.cameraDistance -= static_cast<float>(yoffset) * zoomSpeed;

    if (mouseControl.cameraDistance < 0.5f) mouseControl.cameraDistance = 0.5f; // Limita la distanza minima della camera
    if (mouseControl.cameraDistance > 12.0f) mouseControl.cameraDistance = 12.0f; // Limita la distanza massima della camera

    updateCameraPosition();
}

// Callback inizializzazione piking system
void initializePickingSystem() {
    if (!pickingBuffer.initialize(windoWidth, windowHeight)) {
        std::cerr << "Errore nell'inizializzazione del buffer di picking." << std::endl;
    }

    pickingShader = LoadShader(
        "Shaders/Picking.vert",
        "Shaders/Picking.frag"
    );

    if (pickingShader == 0) {
        std::cerr << "Errore nel caricamento dello shader di picking." << std::endl;
        return;
    }
}

// Funzione per inizializzare il sistema di rendering istanziato
void initializeInstancedRendering() {
    // Inizializza il manager per i modelli istanziati
    auto cubeModel = std::make_shared<CubeModel>();
    cubeModel->initialize();
    instancedModelManager.initialize(cubeModel);

    // Crea alcune istanze iniziali del modello cubo
    instancedModelManager.addInstance(glm::vec3(-2.0f, 0.5f, 0.0f));
    instancedModelManager.addInstance(glm::vec3(0.0f, 0.5f, 0.0f));
    instancedModelManager.addInstance(glm::vec3(2.0f, 0.5f, 0.0f));

    // Aggiorna il buffer delle matrici
    instancedModelManager.updateMatrixBuffer();
}

// Funzione per renderizzare il pannello di controllo delle istanze in ImGui
void renderInstanceControlPanel() {
    if (ImGui::CollapsingHeader("Instanced Models", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Toggle per attivare/disattivare il rendering istanziato
        if (ImGui::Checkbox("Use Instanced Rendering", &useInstancing)) {
            if (useInstancing && !instancedModelManager.isInitialized()) {
                initializeInstancedRendering();
            }
        }

        if (!useInstancing) {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Instanced models hidden (traditional models still visible)");
            return;
        }
        else {
            ImGui::Text("Both instanced and traditional models visible");
        }

        // Numero di istanze
        int instanceCount = instancedModelManager.getInstanceCount();
        ImGui::Text("Instance Count: %d", instanceCount);

        // Pulsante per aggiungere una nuova istanza
        if (ImGui::Button("Add Instance", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) {
            // Genera una posizione casuale
            float randX = (float)rand() / RAND_MAX * 4.0f - 2.0f;  // -2 a +2
            float randZ = (float)rand() / RAND_MAX * 4.0f - 2.0f;  // -2 a +2

            // Aggiungi l'istanza
            unsigned int id = instancedModelManager.addInstance(glm::vec3(randX, 0.5f, randZ));

            // Seleziona automaticamente la nuova istanza
            instancedModelManager.selectInstance(id);
            objectSelected = true;
            showSelectedModelPanel = true;

            // Aggiorna le variabili globali
            ModelInstance* instance = instancedModelManager.getSelectedInstance();
            if (instance) {
                selectedObjectPosition = instance->position;
                modelScale = instance->scale;
                mouseControl.rotationX = instance->rotation.x;
                mouseControl.rotationY = instance->rotation.y;
            }
        }

        // Pulsante per rimuovere l'istanza selezionata
        if (objectSelected && ImGui::Button("Remove Selected Instance", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) {
            ModelInstance* instance = instancedModelManager.getSelectedInstance();
            if (instance) {
                instancedModelManager.removeInstance(instance->instanceId);
                objectSelected = false;
                showSelectedModelPanel = false;
            }
        }

        // Toggle per il picking
        ImGui::Checkbox("Enable Picking", &pickingEnabled);
    }
}

// Implementazione delle nuove funzioni per il SceneManager
void renderScene(GLuint shader, const glm::mat4& view, const glm::mat4& projection) {
    // Renderizza tutti gli oggetti nella scena
    sceneManager.renderAll(shader, view, projection,
        static_cast<Model::RenderMode>(currentRenderMode));
}

void processMousePickingUnified(int x, int y) {
    if (!pickingEnabled || !pickingBuffer.isInitialized()) return;

    pickingBuffer.bind();
    pickingBuffer.clear();

    float aspect = static_cast<float>(windoWidth) / static_cast<float>(windowHeight);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(
        mouseControl.camPos,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glUseProgram(pickingShader);
    SetUniformMat4(pickingShader, "view", view);
    SetUniformMat4(pickingShader, "projection", projection);

    // Renderizza tutti gli oggetti per picking
    sceneManager.renderForPicking(pickingShader, view, projection);

    // Supporto legacy per istanze
    if (useInstancing && instancedModelManager.isInitialized()) {
        instancedModelManager.renderForPicking(pickingShader, view, projection);
    }

    glm::vec3 idColor = pickingBuffer.readPixel(x, y);
    pickingBuffer.unbind();
    glViewport(0, 0, windoWidth, windowHeight);

    handlePickingResultUnified(idColor);
}

void handlePickingResultUnified(const glm::vec3& idColor) {
    // Prima controlla se è un oggetto del SceneManager
    unsigned int objectId = sceneManager.colorToId(idColor);

    if (objectId > 0) {
        sceneManager.selectObject(objectId);
        objectSelected = true;
        showSelectedModelPanel = true;

        auto selected = sceneManager.getSelectedObject();
        if (selected) {
            selectedObjectPosition = selected->getPosition();
            modelScale = selected->getScale();
            mouseControl.rotationX = selected->getRotation().x;
            mouseControl.rotationY = selected->getRotation().y;
        }

        // Deseleziona eventuali istanze legacy
        if (useInstancing) {
            instancedModelManager.deselectAll();
        }
    }
    // Poi prova con il sistema legacy delle istanze
    else if (useInstancing) {
        unsigned int instanceId = instancedModelManager.getInstanceIdFromColor(idColor);
        if (instanceId > 0) {
            // Un'istanza legacy è stata selezionata
            instancedModelManager.selectInstance(instanceId);
            objectSelected = true;
            showSelectedModelPanel = true;

            // Ottieni i dati dell'istanza selezionata
            ModelInstance* instance = instancedModelManager.getSelectedInstance();
            if (instance) {
                selectedObjectPosition = instance->position;
                modelScale = instance->scale;
                mouseControl.rotationX = instance->rotation.x;
                mouseControl.rotationY = instance->rotation.y;
            }

            // Deseleziona gli oggetti del SceneManager
            sceneManager.deselectAll();
        }
        else {
            // Nessun oggetto selezionato
            sceneManager.deselectAll();
            instancedModelManager.deselectAll();
            objectSelected = false;
            showSelectedModelPanel = false;
        }
    }
    else {
        // Nessun oggetto selezionato
        sceneManager.deselectAll();
        objectSelected = false;
        showSelectedModelPanel = false;
    }
}

void renderSceneControlPanel() {
    if (ImGui::CollapsingHeader("Scene Objects", ImGuiTreeNodeFlags_DefaultOpen)) {

        // Pannello per aggiungere oggetti
        ImGui::Text("Add New Object:");
        std::vector<std::string> modelNames = modelManager.getModelNames();

        static int selectedModelIndex = 0;
        if (ImGui::Combo("Model Type", &selectedModelIndex,
            [](void* data, int idx, const char** out_text) {
                auto& names = *static_cast<std::vector<std::string>*>(data);
                if (idx >= 0 && idx < names.size()) {
                    *out_text = names[idx].c_str();
                    return true;
                }
                return false;
            }, &modelNames, modelNames.size())) {
        }

        if (ImGui::Button("Add Object", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30)) &&
            selectedModelIndex < modelNames.size()) {
            // Genera posizione casuale
            float randX = (float)rand() / RAND_MAX * 4.0f - 2.0f;
            float randZ = (float)rand() / RAND_MAX * 4.0f - 2.0f;

            auto newObj = sceneManager.addObject(modelNames[selectedModelIndex],
                glm::vec3(randX, 0.5f, randZ));
            if (newObj) {
                sceneManager.selectObject(newObj->getId());
                objectSelected = true;
                showSelectedModelPanel = true;
                selectedObjectPosition = newObj->getPosition();
                modelScale = newObj->getScale();
            }
        }

        ImGui::Separator();

        // Lista oggetti nella scena
        ImGui::Text("Objects in Scene (%zu):", sceneManager.getObjectCount());

        for (const auto& obj : sceneManager.getObjects()) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (obj->getSelected()) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            ImGui::TreeNodeEx(obj->getName().c_str(), flags);

            if (ImGui::IsItemClicked()) {
                sceneManager.selectObject(obj->getId());
                objectSelected = true;
                showSelectedModelPanel = true;
                selectedObjectPosition = obj->getPosition();
                modelScale = obj->getScale();
                mouseControl.rotationX = obj->getRotation().x;
                mouseControl.rotationY = obj->getRotation().y;
            }

            // Menu contestuale
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete")) {
                    sceneManager.removeObject(obj->getId());
                    if (obj->getSelected()) {
                        objectSelected = false;
                        showSelectedModelPanel = false;
                    }
                }
                ImGui::EndPopup();
            }
        }

        ImGui::Separator();
        ImGui::Checkbox("Enable Picking", &pickingEnabled);
    }
}

int main() {
    // Working directory
    std::cout << std::filesystem::current_path() << std::endl;

    Window win;
    if (!win.initialize(windoWidth, windowHeight, "3D modeler")) return -1;

    // Callbacks
    glfwSetMouseButtonCallback(win.getGLFWwindow(), mouseButtonCallback);
    glfwSetCursorPosCallback(win.getGLFWwindow(), cursorPositionCallback);
    glfwSetScrollCallback(win.getGLFWwindow(), scrollCallback);
    glfwSetFramebufferSizeCallback(win.getGLFWwindow(), framebufferSizeCallback);

    // Inizializza ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(win.getGLFWwindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Carica gli shader
    GLuint shader = LoadShader(
        "Shaders/Object.vert",
        "Shaders/Object.frag"
    );

    GLuint gridShader = LoadShader(
        "Shaders/Grid.vert",
        "Shaders/Grid.frag"
    );

    if (shader == 0 || gridShader == 0) {
        std::cerr << "Errore nel caricamento degli shader." << std::endl;
        return -1;
    }

    // Registra i modelli base
    modelManager.registerModel(std::make_shared<CubeModel>());
    modelManager.registerModel(std::make_shared<SphereModel>());
    modelManager.registerModel(std::make_shared<PyramidModel>());

    // Inizializza i modelli registrati
    modelManager.initializeModels();

    // Inizializza il sistema di picking
    initializePickingSystem();

    // Imposta il modello della griglia
    Grid grid(20, 0.5f);
    grid.initialize();

    // Aggiungi alcuni oggetti iniziali alla scena
    sceneManager.addObject("Cube", glm::vec3(-1.5f, 0.5f, 0.0f));
    sceneManager.addObject("Sphere", glm::vec3(0.0f, 0.5f, 0.0f));
    sceneManager.addObject("Pyramid", glm::vec3(1.5f, 0.5f, 0.0f));

    // Loop temporale
    float timeValue = 0.0f;
    std::string selectedModel = "";

    // Render loop
    while (!win.shouldClose()) {
        // Timer
        static float lastFrame = 0.0f;
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        timeValue += deltaTime;

        // FPS
        frameTimeAccumulator += deltaTime;
        frameCount++;
        if (frameTimeAccumulator >= 0.5f) {
            fps = static_cast<float>(frameCount) * 2.0f; // Moltiplica per 2 perché accumuliamo per 0.5s
            frameTimeAccumulator = 0.0f;
            frameCount = 0;
        }

        // Abilita il test di profondità
        glEnable(GL_DEPTH_TEST);

        // Clear the screen
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Input
        win.processInput();
        win.pollEvents();

        // Aggiorna le matrici una sola volta per frame
        int width, height;
        glfwGetFramebufferSize(win.getGLFWwindow(), &width, &height);
        float aspect = (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glm::mat4 view;

        // Usa la posizione della telecamera calcolata
        view = glm::lookAt(
            mouseControl.camPos,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        // Creazione griglia
        glUseProgram(gridShader);
        glm::mat4 gridModel = glm::mat4(1.0f);
        gridModel = glm::translate(gridModel, glm::vec3(0.0f, -0.01f, 0.0f));
        SetUniformMat4(gridShader, "model", gridModel);
        SetUniformMat4(gridShader, "view", view);
        SetUniformMat4(gridShader, "projection", projection);

        grid.render(gridShader);

        // Renderizza tutti gli oggetti della scena
        renderScene(shader, view, projection);

        // Supporto legacy per rendering istanziato
        if (useInstancing && instancedModelManager.isInitialized()) {
            glUseProgram(shader);
            SetUniformMat4(shader, "view", view);
            SetUniformMat4(shader, "projection", projection);

            GLint colorLocation = glGetUniformLocation(shader, "objectColor");
            if (colorLocation != -1)
                glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f);

            // Applica render mode per le istanze
            applyRenderMode(static_cast<Model::RenderMode>(currentRenderMode));

            // Renderizza le istanze
            instancedModelManager.render(shader, view, projection);

            // Evidenzia l'istanza selezionata
            if (objectSelected) {
                renderSelectedInstanceOutline(shader);
            }

            // Ripristina stato
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        // Generazione frame ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;

        // Creazione del menu ImGui
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(SIDEBAR_WIDTH, displaySize.y), ImGuiCond_Always);
        ImGui::Begin("##Sidebar", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::Text("3D Modeler");
        ImGui::Separator();

        // Pannello principale di controllo della scena
        renderSceneControlPanel();

        // Supporto legacy per rendering istanziato
        renderInstanceControlPanel();

        // Modalità di rendering
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
            static const char* renderModes[] = { "Solid", "Wireframe", "Solid + Wireframe" };
            if (ImGui::Combo("Render Mode", &currentRenderMode, renderModes, IM_ARRAYSIZE(renderModes)))
            {
                // Applicare il cambiamento di modalità di rendering al modello attivo
                Model::RenderMode mode = static_cast<Model::RenderMode>(currentRenderMode);
                if (modelManager.hasActiveModel()) {
                    modelManager.setActiveModelRenderMode(mode);
                }
            }
        }

        // Regolazione distanza telecamera
        if (ImGui::CollapsingHeader("Camera Control", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Camera Distance", &mouseControl.cameraDistance, 0.5f, 12.0f))
            {
                updateCameraPosition();
            }
        }

        // Dati modello selezionato
        if (objectSelected && showSelectedModelPanel) {
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Selected Object", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto selectedObj = sceneManager.getSelectedObject();

                if (selectedObj) {
                    ImGui::Text("Selected Object: %s", selectedObj->getName().c_str());

                    // Posizione
                    ImGui::Separator();
                    ImGui::Text("Position:");

                    glm::vec3 position = selectedObj->getPosition();
                    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25f);
                    bool posChanged = false;
                    posChanged |= ImGui::InputFloat("X##pos", &position.x, 0.1f);
                    ImGui::SameLine();
                    posChanged |= ImGui::InputFloat("Y##pos", &position.y, 0.1f);
                    ImGui::SameLine();
                    posChanged |= ImGui::InputFloat("Z##pos", &position.z, 0.1f);
                    ImGui::PopItemWidth();

                    posChanged |= ImGui::SliderFloat("Pos X", &position.x, -10.0f, 10.0f);
                    posChanged |= ImGui::SliderFloat("Pos Y", &position.y, -10.0f, 10.0f);
                    posChanged |= ImGui::SliderFloat("Pos Z", &position.z, -10.0f, 10.0f);

                    if (posChanged) {
                        selectedObjectPosition = position;
                        sceneManager.updateObjectPosition(selectedObj->getId(), position);
                    }

                    if (ImGui::Button("Reset Position", ImVec2(ImGui::GetWindowWidth() * 0.9f, 30))) {
                        position = glm::vec3(0.0f, 0.5f, 0.0f);
                        selectedObjectPosition = position;
                        sceneManager.updateObjectPosition(selectedObj->getId(), position);
                    }

                    // Scala
                    ImGui::Separator();
                    ImGui::Text("Scale");

                    glm::vec3 scale = selectedObj->getScale();
                    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25f);
                    bool scaleChanged = false;
                    scaleChanged |= ImGui::InputFloat("X##scale", &scale.x, 0.1f);
                    ImGui::SameLine();
                    scaleChanged |= ImGui::InputFloat("Y##scale", &scale.y, 0.1f);
                    ImGui::SameLine();
                    scaleChanged |= ImGui::InputFloat("Z##scale", &scale.z, 0.1f);
                    ImGui::PopItemWidth();

                    // Checkbox per mantenere le proporzioni
                    static bool maintainProportions = true;
                    ImGui::Checkbox("Maintain proportions", &maintainProportions);

                    // Se mantiene le proporzioni, aggiorna tutti i valori quando uno cambia
                    if (maintainProportions && scaleChanged) {
                        static float lastScaleX = 1.0f;
                        static float lastScaleY = 1.0f;
                        static float lastScaleZ = 1.0f;

                        if (scale.x != lastScaleX) {
                            float ratio = scale.x / lastScaleX;
                            scale.y *= ratio;
                            scale.z *= ratio;
                        }
                        else if (scale.y != lastScaleY) {
                            float ratio = scale.y / lastScaleY;
                            scale.x *= ratio;
                            scale.z *= ratio;
                        }
                        else if (scale.z != lastScaleZ) {
                            float ratio = scale.z / lastScaleZ;
                            scale.x *= ratio;
                            scale.y *= ratio;
                        }

                        lastScaleX = scale.x;
                        lastScaleY = scale.y;
                        lastScaleZ = scale.z;
                    }

                    if (scaleChanged) {
                        modelScale = scale;
                        sceneManager.updateObjectScale(selectedObj->getId(), scale);
                    }

                    if (ImGui::Button("Reset Scale", ImVec2(ImGui::GetWindowWidth() * 0.9f, 30))) {
                        scale = glm::vec3(1.0f, 1.0f, 1.0f);
                        modelScale = scale;
                        sceneManager.updateObjectScale(selectedObj->getId(), scale);
                    }

                    // Rotazione
                    ImGui::Separator();
                    ImGui::Text("Rotation");

                    glm::vec3 rotation = selectedObj->getRotation();
                    bool rotChanged = false;
                    rotChanged |= ImGui::SliderFloat("Rot X", &rotation.x, 0.0f, 360.0f);
                    rotChanged |= ImGui::SliderFloat("Rot Y", &rotation.y, 0.0f, 360.0f);
                    rotChanged |= ImGui::SliderFloat("Rot Z", &rotation.z, 0.0f, 360.0f);

                    if (rotChanged) {
                        mouseControl.rotationX = rotation.x;
                        mouseControl.rotationY = rotation.y;
                        sceneManager.updateObjectRotation(selectedObj->getId(), rotation);
                    }

                    if (ImGui::Button("Reset Rotation", ImVec2(ImGui::GetWindowWidth() * 0.9f, 30))) {
                        rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                        mouseControl.rotationX = 0.0f;
                        mouseControl.rotationY = 0.0f;
                        sceneManager.updateObjectRotation(selectedObj->getId(), rotation);
                    }
                }
                // Supporto per istanze legacy
                else if (useInstancing) {
                    ModelInstance* instance = instancedModelManager.getSelectedInstance();
                    if (instance) {
                        ImGui::Text("Selected Instance ID: %u", instance->instanceId);

                        ImGui::Separator();
                        ImGui::Text("Position:");

                        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25f);
                        bool posChanged = false;
                        posChanged |= ImGui::InputFloat("X##pos", &instance->position.x, 0.1f);
                        ImGui::SameLine();
                        posChanged |= ImGui::InputFloat("Y##pos", &instance->position.y, 0.1f);
                        ImGui::SameLine();
                        posChanged |= ImGui::InputFloat("Z##pos", &instance->position.z, 0.1f);
                        ImGui::PopItemWidth();

                        posChanged |= ImGui::SliderFloat("Pos X", &instance->position.x, -10.0f, 10.0f);
                        posChanged |= ImGui::SliderFloat("Pos Y", &instance->position.y, -10.0f, 10.0f);
                        posChanged |= ImGui::SliderFloat("Pos Z", &instance->position.z, -10.0f, 10.0f);

                        if (posChanged) {
                            selectedObjectPosition = instance->position;
                            instancedModelManager.updateInstancePosition(instance->instanceId, instance->position);
                        }

                        if (ImGui::Button("Reset Position", ImVec2(ImGui::GetWindowWidth() * 0.9f, 30))) {
                            instance->position = glm::vec3(0.0f, 0.5f, 0.0f);
                            selectedObjectPosition = instance->position;
                            instancedModelManager.updateInstancePosition(instance->instanceId, instance->position);
                        }

                        // Sezione dimensione
                        ImGui::Separator();
                        ImGui::Text("Scale");

                        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25f);
                        bool scaleChanged = false;
                        scaleChanged |= ImGui::InputFloat("X##scale", &instance->scale.x, 0.1f);
                        ImGui::SameLine();
                        scaleChanged |= ImGui::InputFloat("Y##scale", &instance->scale.y, 0.1f);
                        ImGui::SameLine();
                        scaleChanged |= ImGui::InputFloat("Z##scale", &instance->scale.z, 0.1f);
                        ImGui::PopItemWidth();

                        // Checkbox per mantenere le proporzioni
                        static bool maintainProportions = true;
                        ImGui::Checkbox("Maintain proportions", &maintainProportions);

                        // Se mantiene le proporzioni, aggiorna tutti i valori quando uno cambia
                        if (maintainProportions && scaleChanged) {
                            static float lastScaleX = 1.0f;
                            static float lastScaleY = 1.0f;
                            static float lastScaleZ = 1.0f;

                            if (instance->scale.x != lastScaleX) {
                                float ratio = instance->scale.x / lastScaleX;
                                instance->scale.y *= ratio;
                                instance->scale.z *= ratio;
                            }
                            else if (instance->scale.y != lastScaleY) {
                                float ratio = instance->scale.y / lastScaleY;
                                instance->scale.x *= ratio;
                                instance->scale.z *= ratio;
                            }
                            else if (instance->scale.z != lastScaleZ) {
                                float ratio = instance->scale.z / lastScaleZ;
                                instance->scale.x *= ratio;
                                instance->scale.y *= ratio;
                            }

                            lastScaleX = instance->scale.x;
                            lastScaleY = instance->scale.y;
                            lastScaleZ = instance->scale.z;
                        }

                        if (scaleChanged) {
                            modelScale = instance->scale;
                            instancedModelManager.updateInstanceScale(instance->instanceId, instance->scale);
                        }

                        if (ImGui::Button("Reset Scale", ImVec2(ImGui::GetWindowWidth() * 0.9f, 30))) {
                            instance->scale = glm::vec3(1.0f, 1.0f, 1.0f);
                            modelScale = instance->scale;
                            instancedModelManager.updateInstanceScale(instance->instanceId, instance->scale);
                        }

                        // Sezione rotazione
                        ImGui::Separator();
                        ImGui::Text("Rotation");

                        bool rotChanged = false;
                        rotChanged |= ImGui::SliderFloat("Rot X", &instance->rotation.x, 0.0f, 360.0f);
                        rotChanged |= ImGui::SliderFloat("Rot Y", &instance->rotation.y, 0.0f, 360.0f);
                        rotChanged |= ImGui::SliderFloat("Rot Z", &instance->rotation.z, 0.0f, 360.0f);

                        if (rotChanged) {
                            mouseControl.rotationX = instance->rotation.x;
                            mouseControl.rotationY = instance->rotation.y;
                            instancedModelManager.updateInstanceRotation(instance->instanceId, instance->rotation);
                        }

                        if (ImGui::Button("Reset Rotation", ImVec2(ImGui::GetWindowWidth() * 0.9f, 30))) {
                            instance->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                            mouseControl.rotationX = 0.0f;
                            mouseControl.rotationY = 0.0f;
                            instancedModelManager.updateInstanceRotation(instance->instanceId, instance->rotation);
                        }
                    }
                }
            }
        }

        // Ridimensionamento della finestra
        if (windowResized) {
            projection = glm::perspective(glm::radians(45.0f),
                static_cast<float>(windoWidth) / static_cast<float>(windowHeight), 0.1f, 100.0f);

            ImGui::GetIO().DisplaySize = ImVec2(
                static_cast<float>(windoWidth),
                static_cast<float>(windowHeight)
            );

            // Ridimensiona il buffer di picking
            pickingBuffer.resize(windoWidth, windowHeight);

            windowResized = false;
        }

        bool imguiWantCaptureMouse = ImGui::GetIO().WantCaptureMouse;
        bool imguiWantCaptureKeyboard = ImGui::GetIO().WantCaptureKeyboard;

        // Controllo del mouse e della tastiera
        if (!imguiWantCaptureMouse && !imguiWantCaptureKeyboard) {
            win.processInput();
        }

        // Verifica se la finestra è stata ridimensionata
        glfwGetFramebufferSize(win.getGLFWwindow(), &width, &height);
        if (width == 0 || height == 0) {
            glfwWaitEvents();
            continue;
        }

        // Pulsante help
        ImGui::Separator();
        if (ImGui::Button("Help command window", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) {
            shoeHelpWindow = !shoeHelpWindow;
        }

        ImGui::End(); // Fine sidebar

        // Finestra help
        if (shoeHelpWindow) {
            ImGui::SetNextWindowPos(ImVec2(SIDEBAR_WIDTH + 10, 10), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Commands help", &shoeHelpWindow, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Main commands for 3D modeler");
                ImGui::Separator();

                ImGui::Text("Cam navigation:");
                ImGui::BulletText("Orbital motion: left ALT + left click");
                ImGui::BulletText("Zoom in and out: scroll wheel");

                ImGui::Separator();
                ImGui::Text("Model manipulation:");
                ImGui::BulletText("Select model: left click on the model");
                ImGui::BulletText("Move model: left SHIFT + left click and drag");
                ImGui::BulletText("Move on X axis: left SHIFT + Q + left click and drag");
                ImGui::BulletText("Move on Y axis: left SHIFT + W + left click and drag");
                ImGui::BulletText("Move on Z axis: left SHIFT + E + left click and drag");
                ImGui::BulletText("Rotate model: left click and drag");
                ImGui::BulletText("Change render mode: select from the dropdown menu");

                ImGui::Separator();
                ImGui::Text("Scene management:");
                ImGui::BulletText("Add object: select type and click 'Add Object'");
                ImGui::BulletText("Select object: click on an object in the scene list");
                ImGui::BulletText("Delete object: right-click on object in list and select Delete");

                ImGui::Separator();
                if (ImGui::Button("Close", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) {
                    shoeHelpWindow = false;
                }

                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    shoeHelpWindow = false;
                }

                ImGui::End();
            }
        }

        // FPS display
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 120, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(110, 0), ImGuiCond_Always);
        ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("FPS: %.1f", fps);

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        win.swapBuffers();
    }

    // Pulisci le risorse
    modelManager.cleanup();
    instancedModelManager.cleanup();
    pickingBuffer.cleanup();

    // Chiudi ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteProgram(shader);
    glDeleteProgram(gridShader);
    glDeleteProgram(pickingShader);

    glfwTerminate();
    return 0;
}