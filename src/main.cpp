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
#include <ImGuizmo.h>

// Modelli
#include "Include/Model.h"
#include "../Assets/Include/CubeModel.h"
#include "../Assets/Include/SphereModel.h"
#include "../Assets/Include/PyramidModel.h"
#include "Include/Grid.h"
#include "Include/ModelManager.h"
#include "Include/SceneManager.h"
#include "../Include/ModelLoader.h"
#include "Include/TextureManager.h"

// Aggiungi variabili globali per ImGuizmo
static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::UNIVERSAL;
static ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;

// Aggiungi queste variabili globali vicino alle altre variabili di ImGuizmo
static bool useUniformScaling = true;
static float snapValues[3] = { 0.1f, 0.1f, 0.1f }; // Valori di snap per posizione, rotazione, scala
static bool useSnap = false;

// Forward declarations (aggiungi la nuova funzione)
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
void renderImGuizmo(const glm::mat4& view, const glm::mat4& projection); // Nuova funzione
void openModelFile();
void openTextureFile();

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
float timeValue = static_cast<float>(glfwGetTime());

// Oggetti e controlli
bool objectMoving = false;
glm::vec3 lastMousePos;
float moveSpeed = 0.1f;
bool objectSelected = false;
unsigned int objectToDeleteId = 0;
bool showDeleteConfirmation = false;
static glm::vec3 displayedEulerAngles(0.0f);

// Caricamento file
bool showLoadModelDialog = false;
bool showLoadTextureDialog = false;

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
    } else {      
        yOffset = 1.0f;
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
ImVec2 selectedPanelSize = ImVec2(350, 0);  // Dimensione del pannello

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
        
        // Se l'oggetto selezionato è cambiato, aggiorna gli angoli di visualizzazione
        if (sceneManager.getSelectedObjectId() != objectId) {
            sceneManager.selectObject(objectId);
            objectSelected = true;
            showSelectedModelPanel = true;
            
            // Inizializza gli angoli di visualizzazione dal quaternione dell'oggetto
            auto selectedObj = sceneManager.getSelectedObject();
            if(selectedObj) {
                displayedEulerAngles = glm::degrees(glm::eulerAngles(selectedObj->getRotation()));
            }
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
    // Prima verifichiamo se il mouse è sopra ImGui o ImGuizmo
    if (ImGui::GetIO().WantCaptureMouse || ImGuizmo::IsOver()) {
        return; // Non gestire il click se ImGui o ImGuizmo lo vogliono catturare
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

    if (mouseControl.isOrbiting) {
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

// Implementazione corretta della funzione renderScene
void renderScene(GLuint shader, const glm::mat4& view, const glm::mat4& projection) {
    // Configurazione uniforme degli shader
    glUseProgram(shader);
    SetUniformMat4(shader, "view", view);
    SetUniformMat4(shader, "projection", projection);
    SetUniformInt(shader, "textureSampler", 0);  // Texture sempre nello slot 0
    SetUniformVec3(shader, "viewPos", mouseControl.camPos);
    
    // Imposta il tempo per animazioni
    SetUniformFloat(shader, "time", timeValue);
    
    // Renderizza tutti gli oggetti nella scena
    for (const auto& obj : sceneManager.getObjects()) {
        // Imposta la matrice del modello
        glm::mat4 modelMatrix = obj->getModelMatrix();
        SetUniformMat4(shader, "model", modelMatrix);
        
        // Imposta le uniform per il colore di override
        if (obj->hasOverrideColor()) {
            SetUniformInt(shader, "useOverrideColor", 1);
            SetUniformVec4(shader, "overrideColor", obj->getOverrideColor());
        } else {
            SetUniformInt(shader, "useOverrideColor", 0);
        }

        // Ottieni il modello
        std::shared_ptr<Model> model = obj->getModel();
        
        // Assicurati che la texture corretta sia attiva
        glActiveTexture(GL_TEXTURE0);
        
        // Se il modello ha una texture specifica, usala
        if (model->hasTexture()) {
            glBindTexture(GL_TEXTURE_2D, model->getTextureID());
        } else {
            // Altrimenti, usa la texture di default
            glBindTexture(GL_TEXTURE_2D, TextureManager::getInstance().getDefaultTexture());
        }

        // Renderizza il modello
        obj->getModel()->render();
        
        // Scollega esplicitamente la texture per evitare interferenze
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void renderSceneControlPanel() {
    if (ImGui::CollapsingHeader("Scene Objects", ImGuiTreeNodeFlags_DefaultOpen)) {

        // Pannello per aggiungere oggetti
        ImGui::Text("Add Objects:");
        
        // Aggiungi pulsanti per caricare modelli esterni
        if (ImGui::Button("Load 3D Model", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) {
            openModelFile();
        }
        
        if (ImGui::Button("Load Texture", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) {
            openTextureFile();
        }
        
        ImGui::Separator();
        
        // Codice esistente per aggiungere modelli predefiniti
        ImGui::Text("Add Built-in Object:");
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
                std::cout << "Tentativo di aggiunta modello: " << modelName << std::endl;
                
                glm::vec3 spawnPos = calculateSpawnPosition(modelName, mouseControl.cameraTarget, sceneManager);
                
                std::cout << "Posizione calcolata: (" << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << ")" << std::endl;

                auto newObj = sceneManager.addObject(modelName, spawnPos);
                if (newObj) {
                    std::cout << "Oggetto creato con successo, ID: " << newObj->getId() << std::endl;
                    sceneManager.selectObject(newObj->getId());
                    objectSelected = true;
                    showSelectedModelPanel = true;
                } else {
                    std::cerr << "ERRORE: Creazione oggetto fallita!" << std::endl;
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

    // Gestisci prima le combinazioni con modificatori
    bool ctrlPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                       glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    bool shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    
    static bool keyPressedCtrlA = false;
    static bool keyPressedCtrlD = false;
    static bool keyPressedDelete = false;
    static bool keyPressedT = false;
    static bool keyPressedR = false;
    static bool keyPressedS = false;
    static bool keyPressedU = false; // Per la modalità Universale
    
    // Modalità di ImGuizmo con singoli tasti
    if (objectSelected) {
        // Tasto T: modalità traslazione
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !keyPressedT) {
            currentGizmoOperation = ImGuizmo::TRANSLATE;
            keyPressedT = true;
        } else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
            keyPressedT = false;
        }
        
        // Tasto R: modalità rotazione
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !keyPressedR) {
            currentGizmoOperation = ImGuizmo::ROTATE;
            keyPressedR = true;
        } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
            keyPressedR = false;
        }
        
        // Tasto S: modalità scala
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !keyPressedS) {
            currentGizmoOperation = ImGuizmo::SCALE;
            keyPressedS = true;
        } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE) {
            keyPressedS = false;
        }

        // Tasto U: modalità universale
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS && !keyPressedU) {
            currentGizmoOperation = ImGuizmo::UNIVERSAL;
            keyPressedU = true;
        } else if (glfwGetKey(window, GLFW_KEY_U) == GLFW_RELEASE) {
            keyPressedU = false;
        }
    }
}

void openModelFile() {
    glm::vec3 spawnPos = sceneManager.findValidSpawnPosition(
        mouseControl.camPos, 
        mouseControl.cameraTarget, 
        0.5f,
        1.0f 
    );
    
    ModelLoader::openModelFile(modelManager, sceneManager,
        mouseControl.camPos, spawnPos,
        objectSelected, showSelectedModelPanel);
}

void openTextureFile() {
    ModelLoader::openTextureFile(modelManager, sceneManager);
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
        
        // Renderizza ImGuizmo prima di ImGui ma dopo ImGui::NewFrame()
        renderImGuizmo(view, projection);
        
        // Creazione del menu ImGui
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(SIDEBAR_WIDTH, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);
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
            // Usa PushItemWidth per impostare la larghezza relativa
            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.7f);
            if (ImGui::SliderFloat("Distance", &mouseControl.cameraDistance, 0.5f, 15.0f)) {
                updateCameraPosition();
            }
            ImGui::PopItemWidth();

            if (ImGui::Button("Reset View", ImVec2(-1, 0))) { // -1 per usare tutta la larghezza
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
            
            // Imposta una dimensione minima garantita per il pannello
            ImVec2 minSize = ImVec2(400, 550);
            ImGui::SetNextWindowSizeConstraints(minSize, ImVec2(FLT_MAX, FLT_MAX));
            
            ImGui::SetNextWindowPos(ImVec2(displaySize.x - selectedPanelSize.x - 10, 50), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(selectedPanelSize, ImGuiCond_FirstUseEver);
            
            // Inizia il pannello dell'oggetto selezionato
            if (ImGui::Begin("Selected Object", &showSelectedModelPanel)) {
                auto selectedObj = sceneManager.getSelectedObject();

                if (selectedObj) {
                    // Aggiungi un selettore per l'operazione di ImGuizmo con bottoni on/off
                    ImGui::Text("Transformation Mode:");
                    
                    // Pulsanti per modalità con stile toggle
                    bool isTranslate = currentGizmoOperation == ImGuizmo::TRANSLATE;
                    bool isRotate = currentGizmoOperation == ImGuizmo::ROTATE;
                    bool isScale = currentGizmoOperation == ImGuizmo::SCALE;
                    bool isUniversal = currentGizmoOperation == ImGuizmo::UNIVERSAL;
                    
                    // Stile per pulsanti attivi/inattivi
                    ImGui::PushStyleColor(ImGuiCol_Button, isTranslate ? ImVec4(0.6f, 0.6f, 1.0f, 1.0f) : ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
                    if (ImGui::Button("T", ImVec2(30, 30))) {
                        // Toggle tra TRANSLATE e UNIVERSAL
                        currentGizmoOperation = (isTranslate) ? ImGuizmo::UNIVERSAL : ImGuizmo::TRANSLATE;
                    }
                    ImGui::PopStyleColor();
                    
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, isRotate ? ImVec4(0.6f, 0.6f, 1.0f, 1.0f) : ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
                    if (ImGui::Button("R", ImVec2(30, 30))) {
                        // Toggle tra ROTATE e UNIVERSAL
                        currentGizmoOperation = (isRotate) ? ImGuizmo::UNIVERSAL : ImGuizmo::ROTATE;
                    }
                    ImGui::PopStyleColor();
                    
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, isScale ? ImVec4(0.6f, 0.6f, 1.0f, 1.0f) : ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
                    if (ImGui::Button("S", ImVec2(30, 30))) {
                        // Toggle tra SCALE e UNIVERSAL
                        currentGizmoOperation = (isScale) ? ImGuizmo::UNIVERSAL : ImGuizmo::SCALE;
                    }
                    ImGui::PopStyleColor();
                    
                    // Aggiungere spiegazione della modalità attiva
                    ImGui::SameLine();
                    if (isTranslate) ImGui::Text("Translate Mode");
                    else if (isRotate) ImGui::Text("Rotate Mode");
                    else if (isScale) ImGui::Text("Scale Mode");
                    else ImGui::Text("Universal Mode");
                    
                    // Opzioni di scaling solo se è attiva la modalità scala
                    if (currentGizmoOperation == ImGuizmo::SCALE) {
                        ImGui::Checkbox("Uniform Scaling", &useUniformScaling);
                    }
                    
                    // Opzioni di snap
                    ImGui::Checkbox("Use Snap", &useSnap);
                    if (useSnap) {
                        if (currentGizmoOperation == ImGuizmo::TRANSLATE)
                            ImGui::InputFloat3("Snap", snapValues);
                        else if (currentGizmoOperation == ImGuizmo::ROTATE)
                            ImGui::InputFloat("Angle Snap", &snapValues[1]);
                        else if (currentGizmoOperation == ImGuizmo::SCALE)
                            ImGui::InputFloat("Scale Snap", &snapValues[2]);
                    }
                        
                    ImGui::Separator();
                    
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
                    ImGui::Text("Rotation (Euler Angles):");
                    
                    // Salva gli angoli precedenti
                    glm::vec3 previousAngles = displayedEulerAngles;

                    bool rotChanged = false;

                    // Usa displayedEulerAngles per l'interfaccia
                    columnWidth = ImGui::GetContentRegionAvail().x / 3;
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("X:"); ImGui::SameLine();
                    rotChanged |= ImGui::InputFloat("##rotX", &displayedEulerAngles.x, 1.0f, 0.0f, "%.2f");
                    ImGui::PopItemWidth();

                    ImGui::SameLine();
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("Y:"); ImGui::SameLine();
                    rotChanged |= ImGui::InputFloat("##rotY", &displayedEulerAngles.y, 1.0f, 0.0f, "%.2f");
                    ImGui::PopItemWidth();

                    ImGui::SameLine();
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("Z:"); ImGui::SameLine();
                    rotChanged |= ImGui::InputFloat("##rotZ", &displayedEulerAngles.z, 1.0f, 0.0f, "%.2f");
                    ImGui::PopItemWidth();

                    if (rotChanged) {
                        // Calcola la differenza di rotazione
                        glm::vec3 delta = displayedEulerAngles - previousAngles;
                        
                        // Crea un quaternione dalla differenza e applicalo alla rotazione esistente
                        glm::quat deltaQuat = glm::quat(glm::radians(delta));
                        glm::quat newRotation = deltaQuat * selectedObj->getRotation();
                        
                        sceneManager.updateObjectRotation(selectedObj->getId(), newRotation);
                    }
                    
                    if (ImGui::Button("Reset Rotation", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                        sceneManager.updateObjectRotation(selectedObj->getId(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                        displayedEulerAngles = glm::vec3(0.0f); // Resetta anche gli angoli di visualizzazione
                    }
                    
                    // Aggiungi questo nel pannello Selected Object
                    if (selectedObj) {
                        // Opzione per fissare il pannello
                        ImGui::Checkbox("Pin Panel", &pinSelectedModelPanel);
    
                        // Campo per rinominare l'oggetto
                        static char nameBuffer[128] = "";
                        static bool isEditingName = false;
    
                        if (!isEditingName) {
                            // Mostra nome e pulsante Edit
                            ImGui::Text("Name: %s", selectedObj->getName().c_str());
                            ImGui::SameLine();
                            if (ImGui::Button("Edit")) {
                                isEditingName = true;
                                strncpy(nameBuffer, selectedObj->getName().c_str(), sizeof(nameBuffer) - 1);
                                nameBuffer[sizeof(nameBuffer) - 1] = '\0'; // Assicura terminazione
                            }
                        }
                        else {
                            // Mostra campo di input per modificare il nome
                            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 60);
                            ImGui::InputText("##name_edit", nameBuffer, sizeof(nameBuffer));
                            ImGui::PopItemWidth();

                            ImGui::SameLine();
                            if (ImGui::Button("Apply") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                                sceneManager.renameObject(selectedObj->getId(), nameBuffer);
                                isEditingName = false;
                            }

                            ImGui::SameLine();
                            if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                                isEditingName = false;
                            }
                        }
    
                        ImGui::Separator();
                    }

                    // Aggiungi uno spazio prima del pulsante di eliminazione
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    // Crea uno stile per il pulsante rosso di eliminazione
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));         // Rosso
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));  // Rosso più chiaro
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));   // Rosso più scuro

                    // Pulsante di eliminazione a fondo pannello
                    if (ImGui::Button("Delete Object", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                        showDeleteConfirmation = true;
                        objectToDeleteId = selectedObj->getId();
                    }

                    // Ripristina lo stile originale
                    ImGui::PopStyleColor(3);
                } else {
                    ImGui::Text("No object selected");
                    if (!pinSelectedModelPanel) {
                        showSelectedModelPanel = false;
                    }
                }
                
                // Salva le dimensioni solo se l'utente ha effettivamente ridimensionato la finestra
                if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(0)) {
                    selectedPanelSize = ImGui::GetWindowSize();
                }

                // Aggiungi nella sezione del pannello Selected Object
                if (selectedObj) {
                    // Dopo le sezioni esistenti (posizione, rotazione, scala)
                    ImGui::Separator();
                    ImGui::Text("Texture & Colors:");

                    // Color picker completo
                    static glm::vec4 color = selectedObj->hasOverrideColor() ? selectedObj->getOverrideColor() : glm::vec4(1.0f);
                    if (ImGui::ColorEdit4("Object Color", &color.x)) {
                        selectedObj->setOverrideColor(color);
                    }

                    // Pulsanti per colori predefiniti
                    ImGui::Text("Preset Colors:");
                    ImGui::SameLine();
                    if (ImGui::ColorButton("Red", ImVec4(1.0f, 0.0f, 0.0f, 1.0f))) {
                        selectedObj->setOverrideColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
                    }
                    ImGui::SameLine();
                    if (ImGui::ColorButton("Green", ImVec4(0.0f, 1.0f, 0.0f, 1.0f))) {
                        selectedObj->setOverrideColor(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
                    }
                    ImGui::SameLine();
                    if (ImGui::ColorButton("Blue", ImVec4(0.0f, 0.0f, 1.0f, 1.0f))) {
                        selectedObj->setOverrideColor(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
                    }
                    ImGui::SameLine();
                    if (ImGui::ColorButton("White", ImVec4(1.0f, 1.0f, 1.0f, 1.0f))) {
                        selectedObj->setOverrideColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
                    }

                    // Pulsante per ripristinare la texture/colore originale
                    if (ImGui::Button("Reset to Default", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                        selectedObj->clearOverrideColor();
                    }
                    
                    // Pulsante per caricare textures
                    if (ImGui::Button("Load Texture...", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                        openTextureFile();
                    }
                }
                
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
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            
            // Assicurati di aprire il popup PRIMA di BeginPopupModal
            ImGui::OpenPopup("Delete Object?");
            
            // Imposta lo stile della finestra modale
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
            
            // Usa BeginPopupModal per creare la finestra di conferma
            if (ImGui::BeginPopupModal("Delete Object?", NULL, 
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
                    
                    // Effettua l'eliminazione
                    sceneManager.removeObject(objectToDeleteId);
                    
                    // Resetta lo stato e chiudi il popup
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

// Renderizza ImGuizmo per il modello selezionato
void renderImGuizmo(const glm::mat4& view, const glm::mat4& projection) {
    auto selectedObj = sceneManager.getSelectedObject();
    if (!selectedObj || !objectSelected) {
        return;
    }

    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImGuizmo::SetRect(0, 0, (float)windoWidth, (float)windowHeight);

    glm::mat4 modelMatrix = selectedObj->getModelMatrix();

    ImGuizmo::SetGizmoSizeClipSpace(0.15f);

    // Manipola la matrice con ImGuizmo
    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        currentGizmoOperation,
        currentGizmoMode,
        glm::value_ptr(modelMatrix)
    );

    if (ImGuizmo::IsUsing()) {
        glm::vec3 position, euler, scale;
        glm::quat rotation;
    
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(modelMatrix),
            glm::value_ptr(position),
            glm::value_ptr(euler), // Decomponi in Euler per calcolare il delta
            glm::value_ptr(scale)
        );
    
        // Calcola il delta di rotazione e applicalo agli nostri angoli di visualizzazione
        glm::vec3 deltaRotation = euler - glm::degrees(glm::eulerAngles(selectedObj->getRotation()));
        displayedEulerAngles += deltaRotation;
    
        // Crea il nuovo quaternione dagli angoli di visualizzazione aggiornati
        rotation = glm::quat(glm::radians(displayedEulerAngles));
    
        // Aggiorna l'oggetto
        sceneManager.updateObjectPosition(selectedObj->getId(), position);
        sceneManager.updateObjectRotation(selectedObj->getId(), rotation);
        sceneManager.updateObjectScale(selectedObj->getId(), scale);
    }

};