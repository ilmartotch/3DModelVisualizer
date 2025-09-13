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

    mouseControl.camPos = mouseControl.cameraTarget + glm::vec3(camX, camY + mouseControl.cameraHeight / 2, camZ);
}

// Riposizionamento telecamera all'origine
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

void handlePickingResult(const glm::vec3& idColor) {
    // Controlla se è un oggetto del SceneManager
    unsigned int objectId = sceneManager.colorToId(idColor);

    if (objectId > 0) {
        sceneManager.selectObject(objectId);
        objectSelected = true;
        showSelectedModelPanel = true;
    }
    else {
        // Nessun oggetto selezionato
        sceneManager.deselectAll();
        objectSelected = false;
        showSelectedModelPanel = false;
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
                // Genera posizione casuale
                float randX = (float)rand() / RAND_MAX * 4.0f - 2.0f;
                float randZ = (float)rand() / RAND_MAX * 4.0f - 2.0f;

                auto newObj = sceneManager.addObject(modelNames[selectedModelIndex],
                    glm::vec3(randX, 0.5f, randZ));
                if (newObj) {
                    sceneManager.selectObject(newObj->getId());
                    objectSelected = true;
                    showSelectedModelPanel = true;
                }
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
            }

            // Menu contestuale
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete")) {
                    unsigned int idToRemove = obj->getId();
                    if (obj->getSelected()) {
                        objectSelected = false;
                        showSelectedModelPanel = false;
                    }
                    sceneManager.removeObject(idToRemove);
                    break; // Esce dal loop per evitare iterazione su container modificato
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
    Grid grid(100.0f);
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
                        sceneManager.updateObjectPosition(selectedObj->getId(), position);
                    }

                    if (ImGui::Button("Reset Position", ImVec2(ImGui::GetWindowWidth() * 0.9f, 30))) {
                        position = glm::vec3(0.0f, 0.5f, 0.0f);
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

                    static bool maintainProportions = true;
                    ImGui::Checkbox("Maintain proportions", &maintainProportions);

                    if (scaleChanged) {
                        if (maintainProportions) {
                            static glm::vec3 lastScale = selectedObj->getScale();
                            if (scale.x != lastScale.x) {
                                float ratio = scale.x / lastScale.x;
                                scale.y = ratio; scale.z = ratio;
                            }
                            else if (scale.y != lastScale.y) {
                                float ratio = scale.y / lastScale.y;
                                scale.x = ratio; scale.z = ratio;
                            }
                            else if (scale.z != lastScale.z) {
                                float ratio = scale.z / lastScale.z;
                                scale.x = ratio; scale.y = ratio;
                            }
                        }
                        sceneManager.updateObjectScale(selectedObj->getId(), scale);
                    }

                    if (ImGui::Button("Reset Scale", ImVec2(ImGui::GetWindowWidth() * 0.9f, 30))) {
                        scale = glm::vec3(1.0f, 1.0f, 1.0f);
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
                        sceneManager.updateObjectRotation(selectedObj->getId(), rotation);
                    }

                    if (ImGui::Button("Reset Rotation", ImVec2(ImGui::GetWindowWidth() * 0.9f, 30))) {
                        rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                        sceneManager.updateObjectRotation(selectedObj->getId(), rotation);
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