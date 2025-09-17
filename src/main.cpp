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

// Manager per picking
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
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void updateCameraPosition();
void applyRenderMode(Model::RenderMode mode);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void initializePickingSystem();
void renderScene(GLuint shader, const glm::mat4& view, const glm::mat4& projection);
void renderSceneControlPanel();
void processMousePicking(int x, int y);
void handlePickingResult(const glm::vec3& idColor);
void resetCameraView();
bool isPositionOccupied(const glm::vec3& position, float radius, const SceneManager& sceneManager);
glm::vec3 calculateSpawnPosition(const std::string& modelName, const glm::vec3& cameraTarget, const SceneManager& sceneManager);
void processKeyboardShortcuts(GLFWwindow* window);

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

// Oggetti e controlli
bool objectMoving = false;
glm::vec3 lastMousePos;
float moveSpeed = 0.1f;
bool objectSelected = false;
unsigned int objectToDeleteId = 0;
bool showDeleteConfirmation = false;

std::vector<unsigned int> multiSelectedObjectIds;
bool multiSelectionMode = false;

// Manager per picking
PickingBuffer pickingBuffer;
GLuint pickingShader = 0;
bool pickingEnabled = true;

// Tracciamento RenderMode
int currentRenderMode = static_cast<int>(Model::RenderMode::SOLID);

// Input comandi
struct MouseControl {
    bool isPressed = false;
    bool isOrbiting = false;
    bool isPanning = false;
    float lastX = 0.0f;
    float lastY = 0.0f;
    float cameraDistance = 3.0f;
    float cameraHeight = 1.5f;
    glm::vec3 camPos = glm::vec3(0.0f, 1.5f, 3.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    float orbitalAngleX = 0.0f;
    float orbitalAngleY = 20.0f;
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

// Controlla se una posizione è occupata (versione semplificata)
bool isPositionOccupied(const glm::vec3& position, float radius, const SceneManager& manager) {
    for (const auto& obj : manager.getObjects()) {
        // Semplice controllo della distanza sul piano XZ
        if (glm::distance(glm::vec2(obj->getPosition().x, obj->getPosition().z), glm::vec2(position.x, position.z)) < radius * 2.0f) {
            return true;
        }
    }
    return false;
}

// Calcola la posizione di spawn, gestendo le collisioni
glm::vec3 calculateSpawnPosition(const std::string& modelName, const glm::vec3& cameraTarget, const SceneManager& manager) {
    // Calcola l'offset verticale in base al tipo di modello
    float yOffset = 0.0f;
    if (modelName == "Cube" || modelName == "Sphere") {
        yOffset = 0.5f;
    } else if (modelName == "Pyramid") {
        yOffset = 0.0f;
    }
    
    return manager.findValidSpawnPosition(mouseControl.camPos, cameraTarget, yOffset, 1.0f);
}


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

    mouseControl.camPos = mouseControl.cameraTarget + glm::vec3(camX, camY + mouseControl.cameraHeight / 2, camZ);
}

// Ripristina la vista della telecamera all'origine
void resetCameraView() {
    mouseControl.cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    mouseControl.cameraDistance = 3.0f;
    mouseControl.orbitalAngleX = 0.0f;
    mouseControl.orbitalAngleY = 20.0f;
    updateCameraPosition();
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

// Gestione del picking con ID buffer unificata
void processMousePicking(int x, int y) {
    if (!pickingEnabled || !pickingBuffer.isInitialized()) return;

    pickingBuffer.bind();
    pickingBuffer.clear();

    float aspect = static_cast<float>(windoWidth) / static_cast<float>(windowHeight);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(
        mouseControl.camPos,
        mouseControl.cameraTarget,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glUseProgram(pickingShader);
    SetUniformMat4(pickingShader, "view", view);
    SetUniformMat4(pickingShader, "projection", projection);

    // Renderizza tutti gli oggetti per picking
    sceneManager.renderForPicking(pickingShader, view, projection);

    glm::vec3 idColor = pickingBuffer.readPixel(x, y);
    pickingBuffer.unbind();
    glViewport(0, 0, windoWidth, windowHeight);

    handlePickingResult(idColor);
}

float lastClickTime = 0.0f;
const float doubleClickTimeThreshold = 0.3f; // 300 ms per riconoscere un doppio click
unsigned int lastClickedObjectId = 0;

// Aggiungi queste variabili globali
bool pinSelectedModelPanel = false;  // Per "fissare" il pannello
ImVec2 selectedPanelSize = ImVec2(300, 0);  // Dimensione del pannello

// Modifica la funzione handlePickingResult per gestire la visualizzazione del pannello
void handlePickingResult(const glm::vec3& idColor) {
    // Controlla se è un oggetto del SceneManager
    unsigned int objectId = sceneManager.colorToId(idColor);
    
    if (objectId > 0) {
        // Ottieni il tempo corrente per il controllo del doppio click
        float currentTime = static_cast<float>(glfwGetTime());
        
        // Controlla se questo è un doppio click sullo stesso oggetto
        if (objectId == lastClickedObjectId && 
            (currentTime - lastClickTime) < doubleClickTimeThreshold) {
            // Doppio click rilevato - seleziona l'oggetto e mostra il pannello
            sceneManager.selectObject(objectId);
            objectSelected = true;
            showSelectedModelPanel = true;
            
            // Reset per evitare di rilevare un triplo click come doppio
            lastClickTime = 0.0f;
            lastClickedObjectId = 0;
        } else {
            // Primo click - seleziona l'oggetto e mostra comunque il pannello
            sceneManager.selectObject(objectId);
            objectSelected = true;
            showSelectedModelPanel = true;
            
            // Aggiorna i dati per il potenziale doppio click
            lastClickTime = currentTime;
            lastClickedObjectId = objectId;
        }
    } else {
        // Nessun oggetto selezionato
        sceneManager.deselectAll();
        objectSelected = false;
        
        // Se il pannello non è fissato, lo nascondiamo
        if (!pinSelectedModelPanel) {
            showSelectedModelPanel = false;
        }
        
        // Reset delle variabili di doppio click
        lastClickTime = 0.0f;
        lastClickedObjectId = 0;
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Prima verifichiamo se il mouse è sopra ImGui
    if (ImGui::GetIO().WantCaptureMouse) {
        return; // Non gestire il click se ImGui lo vuole catturare
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            // Controlla se ALT è premuto per modalità orbita
            if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
                mouseControl.isOrbiting = true;
                mouseControl.isPressed = false;
                mouseControl.isPanning = false;
            }
            // Controlla se CTRL è premuto per il panning
            else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
                mouseControl.isPanning = true;
                mouseControl.isOrbiting = false;
                mouseControl.isPressed = false;
            }
            // Logica per la manipolazione degli oggetti
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
                if (pickingEnabled)
                {processMousePicking(static_cast<int>(xpos), static_cast<int>(ypos));}
                mouseControl.isOrbiting = false;
                mouseControl.isPanning = false;
                mouseControl.isPressed = true;
            }

            mouseControl.lastX = (float)xpos;
            mouseControl.lastY = (float)ypos;
        }
        else if (action == GLFW_RELEASE) {
            mouseControl.isPressed = false;
            mouseControl.isOrbiting = false;
            mouseControl.isPanning = false;
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
    auto selectedObj = sceneManager.getSelectedObject();

    if (objectMoving && selectedObj) {
        glm::vec3 currentMousePos = glm::vec3((float)xpos, (float)ypos, 0.0f);
        glm::vec3 mouseDelta = currentMousePos - lastMousePos;
        glm::vec3 newPosition = selectedObj->getPosition();

        // Aggiorna la posizione dell'oggetto selezionato
        switch (currentMoveAxis) {
        case MoveAxis::X_AXIS:
            newPosition.x += mouseDelta.x * moveSpeed;
            break;
        case MoveAxis::Y_AXIS:
            newPosition.y -= mouseDelta.y * moveSpeed;
            break;
        case MoveAxis::Z_AXIS:
            newPosition.z += mouseDelta.y * moveSpeed;
            break;
        case MoveAxis::NONE:
            newPosition.x += mouseDelta.x * moveSpeed;
            newPosition.y -= mouseDelta.y * moveSpeed;
            break;
        }

        lastMousePos = currentMousePos;
        sceneManager.updateObjectPosition(selectedObj->getId(), newPosition);

    }
    else if (mouseControl.isPressed && selectedObj) {
        glm::vec3 newRotation = selectedObj->getRotation();
        newRotation.x += yoffset * sensitivity;
        newRotation.y += xoffset * sensitivity;

        newRotation.x = fmodf(newRotation.x, 360.0f);
        newRotation.y = fmodf(newRotation.y, 360.0f);

        sceneManager.updateObjectRotation(selectedObj->getId(), newRotation);
    }
    else if (mouseControl.isOrbiting) {
        mouseControl.orbitalAngleX += xoffset * sensitivity;
        mouseControl.orbitalAngleY += yoffset * sensitivity;
        if (mouseControl.orbitalAngleY > 85.0f) mouseControl.orbitalAngleY = 85.0f;
        if (mouseControl.orbitalAngleY < -85.0f) mouseControl.orbitalAngleY = -85.0f;

        updateCameraPosition();
    }
    else if (mouseControl.isPanning) {
        const float panSpeed = 0.003f * mouseControl.cameraDistance;

        // Calcola la direzione di vista della camera e proiettala sul piano XZ
        glm::vec3 viewDir = mouseControl.cameraTarget - mouseControl.camPos;
        glm::vec3 forwardOnPlane = glm::normalize(glm::vec3(viewDir.x, 0.0f, viewDir.z));

        // Il vettore "destra" è perpendicolare alla direzione di vista sul piano
        glm::vec3 rightOnPlane = glm::normalize(glm::cross(forwardOnPlane, glm::vec3(0.0f, 1.0f, 0.0f)));

        // Muovi il target della camera solo sul piano XZ
        mouseControl.cameraTarget -= rightOnPlane * xoffset * panSpeed;
        mouseControl.cameraTarget -= forwardOnPlane * yoffset * panSpeed; // Usa la direzione "avanti" sul piano

        // Aggiorna la posizione della camera in base al nuovo target
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
        if (mouseControl.cameraDistance > 15.0f) mouseControl.cameraDistance = 15.0f; // Limita la distanza massima della camera

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

// Implementazione delle nuove funzioni per il SceneManager
void renderScene(GLuint shader, const glm::mat4& view, const glm::mat4& projection) {
    // Renderizza tutti gli oggetti nella scena
    sceneManager.renderAll(shader, view, projection,
        static_cast<Model::RenderMode>(currentRenderMode));
}

void renderSceneControlPanel() {
    if (ImGui::CollapsingHeader("Scene Objects", ImGuiTreeNodeFlags_DefaultOpen)) {

        // Pannello per aggiungere oggetti
        ImGui::Text("Add New Object:");
        std::vector<std::string> modelNames = modelManager.getModelNames();

        static int selectedModelIndex = 0;
        if (!modelNames.empty()) {
            if (ImGui::Combo("Model Type", &selectedModelIndex,
            [](void* data, int idx, const char** out_text) {
                auto& names = *static_cast<std::vector<std::string>*>(data);
                if (idx >= 0 && idx < names.size()) {
                    *out_text = names[idx].c_str();
                    return true;
                }
                return false;
            }, & modelNames, modelNames.size())) {
            }

            if (ImGui::Button("Add Object", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30)) &&
                selectedModelIndex < modelNames.size()) {
                
                const std::string& modelName = modelNames[selectedModelIndex];
                glm::vec3 spawnPos = calculateSpawnPosition(modelName, mouseControl.cameraTarget, sceneManager);

                auto newObj = sceneManager.addObject(modelName, spawnPos);
                if (newObj) {
                    sceneManager.selectObject(newObj->getId());
                    objectSelected = true;
                    showSelectedModelPanel = true;
                }
            }
        }

        ImGui::Separator();

        // Variabili per gestire la rinomina
        static bool isRenaming = false;
        static unsigned int renamingId = 0;
        static char renameBuffer[128] = ""; // Buffer per il nuovo nome

        // Lista oggetti nella scena
        ImGui::Text("Objects in Scene (%zu):", sceneManager.getObjectCount());

        for (const auto& obj : sceneManager.getObjects()) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (obj->getSelected()) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            // Mostra il campo di input per la rinomina o il nome normale
            bool isThisObjectRenaming = isRenaming && renamingId == obj->getId();
            
            if (isThisObjectRenaming) {
                // Mostra il campo di input per rinominare
                ImGui::PushID(static_cast<int>(obj->getId()));
                if (ImGui::InputText("##rename", renameBuffer, sizeof(renameBuffer), 
                                    ImGuiInputTextFlags_EnterReturnsTrue)) {
                    // Applica il nuovo nome quando l'utente preme Enter
                    sceneManager.renameObject(obj->getId(), renameBuffer);
                    isRenaming = false;
                }
                
                // Gestione della perdita di focus
                if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) {
                    isRenaming = false;
                }
                ImGui::PopID();
            } else {
                // Mostra il nome normale
                ImGui::TreeNodeEx(obj->getName().c_str(), flags);
                
                // Controllo per il doppio click
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    isRenaming = true;
                    renamingId = obj->getId();
                    strncpy(renameBuffer, obj->getName().c_str(), sizeof(renameBuffer) - 1);
                    renameBuffer[sizeof(renameBuffer) - 1] = '\0'; // Assicura terminazione
                }
            }

            // Gestione click singolo (selezione)
            if (!isThisObjectRenaming && ImGui::IsItemClicked()) {
                sceneManager.selectObject(obj->getId());
                objectSelected = true;
                showSelectedModelPanel = true;
            }

            // Menu contestuale
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Rename")) {
                    isRenaming = true;
                    renamingId = obj->getId();
                    strncpy(renameBuffer, obj->getName().c_str(), sizeof(renameBuffer) - 1);
                }
                
                if (ImGui::MenuItem("Delete")) {
                    unsigned int idToRemove = obj->getId();
                    
                    // Se l'oggetto da rimuovere è quello attualmente selezionato
                    if (obj->getSelected()) {
                        objectSelected = false;
                        showSelectedModelPanel = false;
                    }
                    
                    // Rimuovi l'oggetto dalla scena
                    sceneManager.removeObject(idToRemove);
                    
                    // Esci dal loop per evitare iterazione su container modificato
                    ImGui::EndPopup();
                    break;
                }
                ImGui::EndPopup();
            }
        }

        ImGui::Separator();
        ImGui::Checkbox("Enable Picking", &pickingEnabled);
    }
}

void processKeyboardShortcuts(GLFWwindow* window) {
    // Salta se ImGui sta usando l'input
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    static bool keyPressedCtrlA = false;
    static bool keyPressedCtrlD = false;
    static bool keyPressedDelete = false;
    
    // CTRL+A: Seleziona tutti gli oggetti
    if ((glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) && 
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        
        if (!keyPressedCtrlA) {
            keyPressedCtrlA = true;
            
            // Seleziona tutti gli oggetti
            multiSelectedObjectIds.clear();
            for (const auto& obj : sceneManager.getObjects()) {
                multiSelectedObjectIds.push_back(obj->getId());
            }
            multiSelectionMode = true;
            
            // Feedback visivo
            std::cout << "Selected " << multiSelectedObjectIds.size() << " objects." << std::endl;
        }
    } else {
        keyPressedCtrlA = false;
    }
    
    // CTRL+D: Duplica l'oggetto selezionato
    if ((glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) && 
        glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        
        if (!keyPressedCtrlD && objectSelected) {
            keyPressedCtrlD = true;
            
            auto selectedObj = sceneManager.getSelectedObject();
            if (selectedObj) {
                // Crea una copia dell'oggetto con un offset di posizione
                glm::vec3 newPos = selectedObj->getPosition() + glm::vec3(0.5f, 0.0f, 0.5f);
                auto newObj = sceneManager.duplicateObject(selectedObj->getId(), newPos);
                
                if (newObj) {
                    // Seleziona il nuovo oggetto
                    sceneManager.selectObject(newObj->getId());
                    objectSelected = true;
                    showSelectedModelPanel = true;
                }
            }
        }
    } else {
        keyPressedCtrlD = false;
    }
    
    // CANC: Elimina l'oggetto selezionato o gli oggetti multi-selezionati
    if (glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS) {
        if (!keyPressedDelete) {
            keyPressedDelete = true;
            
            if (multiSelectionMode && !multiSelectedObjectIds.empty()) {
                // Chiedi conferma per eliminazione multipla
                showDeleteConfirmation = true;
                // Implementa la logica di eliminazione multipla
            }
            else if (objectSelected) {
                // Usa il sistema di conferma già implementato
                auto selectedObj = sceneManager.getSelectedObject();
                if (selectedObj) {
                    showDeleteConfirmation = true;
                    objectToDeleteId = selectedObj->getId();
                }
            }
        }
    } else {
        keyPressedDelete = false;
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
    Grid grid;
    grid.initialize();

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
            fps = static_cast<float>(frameCount) * 2.0f;
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
        
        // Gestione scorciatoie da tastiera
        processKeyboardShortcuts(win.getGLFWwindow());

        // Aggiorna le matrici una sola volta per frame
        int width, height;
        glfwGetFramebufferSize(win.getGLFWwindow(), &width, &height);
        float aspect = (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glm::mat4 view;

        // Usa la posizione della telecamera calcolata
        view = glm::lookAt(
            mouseControl.camPos,
            mouseControl.cameraTarget,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        // Creazione griglia
        grid.render(gridShader, projection, view, mouseControl.camPos);

        // Renderizza tutti gli oggetti della scena
        renderScene(shader, view, projection);

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

        // Modalità di rendering
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
            static const char* renderModes[] = { "Solid", "Wireframe", "Solid + Wireframe" };
            if (ImGui::Combo("Render Mode", &currentRenderMode, renderModes, IM_ARRAYSIZE(renderModes)))
            {
                Model::RenderMode mode = static_cast<Model::RenderMode>(currentRenderMode);
                if (modelManager.hasActiveModel()) {
                    modelManager.setActiveModelRenderMode(mode);
                }
            }
        }

        // Regolazione distanza telecamera
        if (ImGui::CollapsingHeader("Camera Control", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Camera Distance", &mouseControl.cameraDistance, 0.5f, 15.0f))
            {
                updateCameraPosition();
            }

            if (ImGui::Button("Reset View", ImVec2(ImGui::GetWindowWidth() * 0.9f, 30))) {
                resetCameraView();
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

            pickingBuffer.resize(windoWidth, windowHeight);
            windowResized = false;
        }

        bool imguiWantCaptureMouse = ImGui::GetIO().WantCaptureMouse;
        bool imguiWantCaptureKeyboard = ImGui::GetIO().WantCaptureKeyboard;

        if (!imguiWantCaptureMouse && !imguiWantCaptureKeyboard) {
            win.processInput();
        }

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

        // Pannello per il modello selezionato (fuori dalla sidebar)
        if (showSelectedModelPanel) {
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            ImGui::SetNextWindowPos(ImVec2(displaySize.x - selectedPanelSize.x - 10, 50), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(selectedPanelSize, ImGuiCond_FirstUseEver);
            
            if (ImGui::Begin("Selected Object", &showSelectedModelPanel)) {
                auto selectedObj = sceneManager.getSelectedObject();

                if (selectedObj) {
                    // Opzione per fissare il pannello
                    ImGui::Checkbox("Pin Panel", &pinSelectedModelPanel);
                    
                    ImGui::Text("Selected: %s", selectedObj->getName().c_str());
                    ImGui::Separator();
                    
                    // Posizione - solo input fields, no sliders
                    ImGui::Text("Position:");
                    glm::vec3 position = selectedObj->getPosition();
                    
                    bool posChanged = false;
                    
                    // Layout input X, Y, Z
                    float columnWidth = ImGui::GetContentRegionAvail().x / 3;
                    
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("X:"); ImGui::SameLine();
                    posChanged |= ImGui::InputFloat("##posX", &position.x, 0.1f);
                    ImGui::PopItemWidth();
                    
                    ImGui::SameLine();
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("Y:"); ImGui::SameLine();
                    posChanged |= ImGui::InputFloat("##posY", &position.y, 0.1f);
                    ImGui::PopItemWidth();
                    
                    ImGui::SameLine();
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("Z:"); ImGui::SameLine();
                    posChanged |= ImGui::InputFloat("##posZ", &position.z, 0.1f);
                    ImGui::PopItemWidth();
                    
                    if (posChanged) {
                        sceneManager.updateObjectPosition(selectedObj->getId(), position);
                    }
                    
                    if (ImGui::Button("Reset Position", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                        sceneManager.resetObjectToInitialPosition(selectedObj->getId());
                    }
                    
                    // Scala
                    ImGui::Separator();
                    ImGui::Text("Scale:");
                    
                    glm::vec3 scale = selectedObj->getScale();
                    bool scaleChanged = false;
                    
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("X:"); ImGui::SameLine();
                    scaleChanged |= ImGui::InputFloat("##scaleX", &scale.x, 0.1f);
                    ImGui::PopItemWidth();
                    
                    ImGui::SameLine();
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("Y:"); ImGui::SameLine();
                    scaleChanged |= ImGui::InputFloat("##scaleY", &scale.y, 0.1f);
                    ImGui::PopItemWidth();
                    
                    ImGui::SameLine();
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("Z:"); ImGui::SameLine();
                    scaleChanged |= ImGui::InputFloat("##scaleZ", &scale.z, 0.1f);
                    ImGui::PopItemWidth();
                    
                    static bool maintainProportions = true;
                    ImGui::Checkbox("Maintain proportions", &maintainProportions);
                    
                    if (scaleChanged) {
                        if (maintainProportions) {
                            static glm::vec3 lastScale = selectedObj->getScale();
                            if (scale.x != lastScale.x) {
                                float ratio = scale.x / lastScale.x;
                                scale.y = lastScale.y * ratio; 
                                scale.z = lastScale.z * ratio;
                            }
                            else if (scale.y != lastScale.y) {
                                float ratio = scale.y / lastScale.y;
                                scale.x = lastScale.x * ratio; 
                                scale.z = lastScale.z * ratio;
                            }
                            else if (scale.z != lastScale.z) {
                                float ratio = scale.z / lastScale.z;
                                scale.x = lastScale.x * ratio; 
                                scale.y = lastScale.y * ratio;
                            }
                            lastScale = scale;
                        }
                        sceneManager.updateObjectScale(selectedObj->getId(), scale);
                    }
                    
                    if (ImGui::Button("Reset Scale", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                        scale = glm::vec3(1.0f, 1.0f, 1.0f);
                        sceneManager.updateObjectScale(selectedObj->getId(), scale);
                    }
                    
                    // Rotazione
                    ImGui::Separator();
                    ImGui::Text("Rotation:");
                    
                    glm::vec3 rotation = selectedObj->getRotation();
                    bool rotChanged = false;
                    
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("X:"); ImGui::SameLine();
                    rotChanged |= ImGui::InputFloat("##rotX", &rotation.x, 1.0f);
                    ImGui::PopItemWidth();
                    
                    ImGui::SameLine();
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("Y:"); ImGui::SameLine();
                    rotChanged |= ImGui::InputFloat("##rotY", &rotation.y, 1.0f);
                    ImGui::PopItemWidth();
                    
                    ImGui::SameLine();
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("Z:"); ImGui::SameLine();
                    rotChanged |= ImGui::InputFloat("##rotZ", &rotation.z, 1.0f);
                    ImGui::PopItemWidth();
                    
                    if (rotChanged) {
                        // Normalizziamo i valori di rotazione tra 0 e 360 gradi
                        rotation.x = fmodf(rotation.x, 360.0f);
                        if (rotation.x < 0) rotation.x += 360.0f;
                        
                        rotation.y = fmodf(rotation.y, 360.0f);
                        if (rotation.y < 0) rotation.y += 360.0f;
                        
                        rotation.z = fmodf(rotation.z, 360.0f);
                        if (rotation.z < 0) rotation.z += 360.0f;
                        
                        sceneManager.updateObjectRotation(selectedObj->getId(), rotation);
                    }
                    
                    if (ImGui::Button("Reset Rotation", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                        rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                        sceneManager.updateObjectRotation(selectedObj->getId(), rotation);
                    }
                } else {
                    ImGui::Text("No object selected");
                    if (!pinSelectedModelPanel) {
                        showSelectedModelPanel = false;
                    }
                }
                
                // Memorizziamo la dimensione attuale della finestra per il prossimo frame
                selectedPanelSize = ImGui::GetWindowSize();

                // Aggiungi un separatore
                ImGui::Separator();

                // Crea uno stile per il pulsante rosso di eliminazione
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));         // Rosso
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));  // Rosso più chiaro
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));   // Rosso più scuro

                // Pulsante di eliminazione a fondo pannello
                if (ImGui::Button("Delete Object", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    // Invece di eliminare subito, mostra la finestra di conferma
                    showDeleteConfirmation = true;
                    objectToDeleteId = selectedObj->getId();
                }

                // Ripristina lo stile originale
                ImGui::PopStyleColor(3);

                ImGui::End();
            }
        }

        // Finestra help
        if (shoeHelpWindow) {
            ImGui::SetNextWindowPos(ImVec2(SIDEBAR_WIDTH + 10, 10), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Commands help", &shoeHelpWindow, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Main commands for 3D modeler");
                ImGui::Separator();
                ImGui::Text("Cam navigation:");
                ImGui::BulletText("Orbital motion: left ALT + left click and drag");
                ImGui::BulletText("Pan view: left CTRL + left click and drag");
                ImGui::BulletText("Zoom in and out: scroll wheel");
                ImGui::BulletText("Reset view: 'Reset View' button");
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
                ImGui::BulletText("Duplicate object: CTRL+D");
                ImGui::BulletText("Select all objects: CTRL+A");
                ImGui::BulletText("Delete selected: DELETE key");
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

        // Finestra modale di conferma eliminazione
        if (showDeleteConfirmation) {
            // Centra la finestra di dialogo
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(300, 0));
            
            // Imposta lo stile della finestra modale
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
            
            if (ImGui::BeginPopupModal("Delete Object?", &showDeleteConfirmation, 
                                  ImGuiWindowFlags_AlwaysAutoResize | 
                                  ImGuiWindowFlags_NoSavedSettings)) {
                ImGui::Text("Are you sure you want to delete this object?");
                ImGui::Text("This operation cannot be undone.");
                ImGui::Separator();
                
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                
                // Layout con due pulsanti allineati
                float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2;
                
                if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
                    showDeleteConfirmation = false;
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::SameLine();
                
                // Pulsante di conferma in rosso
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
                
                if (ImGui::Button("Delete", ImVec2(buttonWidth, 0))) {
                    // Procedi con l'eliminazione
                    if (sceneManager.getSelectedObjectId() == objectToDeleteId) {
                        objectSelected = false;
                        showSelectedModelPanel = false;
                        pinSelectedModelPanel = false;
                    }
                    
                    sceneManager.removeObject(objectToDeleteId);
                    showDeleteConfirmation = false;
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::PopStyleColor(3);
                
                // Permette anche la chiusura con Esc
                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    showDeleteConfirmation = false;
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::EndPopup();
            }
            
            ImGui::PopStyleVar();
            
            // Apri il popup automaticamente
            if (showDeleteConfirmation) {
                ImGui::OpenPopup("Delete Object?");
            }
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        win.swapBuffers();
    }

    // Pulisci le risorse
    modelManager.cleanup();
    pickingBuffer.cleanup();
    grid.cleanup();

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