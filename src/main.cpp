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
#include <thread>
#include <chrono>
#include <functional>
#include <sstream>
#include <iomanip>
#include <algorithm>

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
#include "../Assets/Include/ImagePlaneModel.h"
#include "Include/Grid.h"
#include "Include/ModelManager.h"
#include "Include/SceneManager.h"
#include "../Include/ModelLoader.h"
#include "Include/TextureManager.h"
#include "Include/AssimpSceneExporter.h"

// Shadows
#include "Include/ShadowSystem.h"
#include "Include/ShadowFloor.h"

// Logger
#include "Include/Logger.h"
#include "Include/ConsoleWindow.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#ifdef _WIN32
#include <windows.h>
#endif

static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::UNIVERSAL;
static ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;

static bool useUniformScaling = true;
static float snapValues[3] = { 0.1f, 0.1f, 0.1f };
static bool useSnap = false;

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
void renderImGuizmo(const glm::mat4& view, const glm::mat4& projection);
void openModelFile();
void openImageFile();
void renderLoadingDialog();
void ApplayModernRoundedStyle();

// Variabili window
int gWindowWidthLogical = 800;
int gWindowHeightLogical = 600;
int gFramebufferWidth = 800;
int gFramebufferHeight = 600;
bool windowResized = false;
static bool shoeHelpWindow = false;
const int SIDEBAR_WIDTH = 300;
bool showModelInfo = true;
bool showSelectedModelPanel = false;
static bool gShowConsole = false;
static bool gShowLogger = false;

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

// Config griglia
static bool gInfiniteGrid = false;
static constexpr float gGridHalfSize = 5.0f;
static constexpr float gGridBoundaryMargin = 1.5f;
static const glm::vec3 GRID_COLOR = glm::vec3(0.0f, 0.0f, 0.0f);
static const glm::vec3 GRID_X_AXIS_COLOR = glm::vec3(1.0f, 0.2f, 0.2f);
static const glm::vec3 GRID_Z_AXIS_COLOR = glm::vec3(0.2f, 0.2f, 1.0f);
static constexpr int gFiniteModeMaxObjects = 10;
static bool gFiniteLimitReached = false;
static bool showFiniteLimitDialog = false;
static bool gLastPlacementClamped = false;
static bool gLastMovementClamped = false;
static bool showFiniteSpaceInfoWindow = false;
static std::string gBoundaryWarningMessage;
static bool showGridModeSwitchErrorPopup = false;
static std::string gGridModeSwitchErrorMessage;

struct TextureReplaceDialog {
    bool show = false;
    std::string texturePath = "";
    GLuint newTextureID = 0;
};

static TextureReplaceDialog textureReplaceDialog;
static bool dontAskTextureReplace = false;

struct LightingSettings {
    glm::vec3 ambientColor{ 0.05f, 0.05f, 0.05f };
    float ambientIntensity = 0.0f;
};

static LightingSettings gLighting;

// Directional light
struct DirectionalLightState {
    glm::vec3 position{ -2.0f, 2.0f, 2.0f };
    glm::vec3 color{ 1.0f, 0.95f, 0.8f }; // Colore luce emessa
    glm::vec3 gradientStart{ 1.0f, 0.9f, 0.6f }; // Colore alto icona
    glm::vec3 gradientEnd{ 1.0f, 0.5f, 0.2f }; // Colore basso icona
    bool useGradient = true;
    bool enabled = true;
    bool enablePlanarShadows = true;
    glm::vec4 shadowColor{0.0f, 0.0f, 0.0f, 0.35f}; 
    bool useTarget = true;
	glm::vec3 target{ 0.0f, 0.0f, 0.0f };
    glm::vec3 orientationEuler{ 20.0f, 45.0f, 0.0f };
};

static DirectionalLightState gDirLight;
static std::string gDirLightObjectId = "DIR_LIGHT_ID";
static constexpr unsigned int gDirLightObjectNumericId = 0x00FFFFFE;
static bool showDirectionalLightWindow = false;
static bool pinDirectionalLightWindow = false;
static bool editingLightName = false;
static char lightNameBuffer[128] = "Directional Light";

// Shadow mapping
static ShadowSystem gShadows;
static const int DIR_SHADOW_SIZE = 4096;
static constexpr float gFloorHeight = 0.0f;
static bool gEnableShadowMap = true;

// FPS
float fps = 0.0f;
float frameTimeAccumulator = 0.0f;
int frameCount = 0;
float gTime = 0.0f;

// Oggetti e controlli
bool objectMoving = false;
glm::vec3 lastMousePos;
float moveSpeed = 0.1f;
bool objectSelected = false;
unsigned int objectToDeleteId = 0;
bool showDeleteConfirmation = false;
static glm::vec3 displayedEulerAngles(0.0f);

static inline glm::vec3 QuantizeIdColor(const glm::vec3& c) {
    return glm::vec3(
        floorf(c.r * 255.0f + 0.5f) / 255.0f,
        floorf(c.g * 255.0f + 0.5f) / 255.0f,
        floorf(c.b * 255.0f + 0.5f) / 255.0f
    );
}

// Caricamento file
bool showLoadModelDialog = false;
bool showLoadTextureDialog = false;

// Gestione errori
struct ErrorDialog
{
    bool show = false;
    std::string title = "";
    std::string message = "";
    std::string details = "";
};
static ErrorDialog errorDialog;

struct SuccessDialog {
    bool show = false;
    std::string title = "";
    std::string message = "";
    std::string details = "";

    void reset() {
        show = false;
        title = "";
        message = "";
        details = "";
    }
};

static SuccessDialog successDialog;

// Dialog di caricamento
struct LoadingDialog {
    bool show = false;
    std::string fileName = "";
    float progress = 0.0f;
    std::string statusMessage = "Loading...";
    float elapsedTime = 0.0f;
    float startTime = 0.0f;
    bool isComplete = false;
    bool hasError = false;
    std::string errorMessage = "";
    
    // Animazione spinner
    float spinnerAngle = 0.0f;
    
    void reset() {
        show = false;
        fileName = "";
        progress = 0.0f;
        statusMessage = "Loading...";
        elapsedTime = 0.0f;
        startTime = 0.0f;
        isComplete = false;
        hasError = false;
        errorMessage = "";
        spinnerAngle = 0.0f;
    }
    
    void start(const std::string& file) {
        reset();
        show = true;
        fileName = file;
        startTime = static_cast<float>(glfwGetTime());
    }
    
    void updateProgress(float prog, const std::string& message = "") {
        progress = std::clamp(prog, 0.0f, 1.0f);
        if (!message.empty()) {
            statusMessage = message;
        }
        elapsedTime = static_cast<float>(glfwGetTime()) - startTime;
    }
    
    void complete() {
        progress = 1.0f;
        statusMessage = "Complete!";
        isComplete = true;
    }
    
    void error(const std::string& msg) {
        hasError = true;
        errorMessage = msg;
        statusMessage = "Error!";
    }
};

static LoadingDialog loadingDialog;

struct ExportFormatDialog {
    bool show = false;
    int selectedFormatIndex = 0;
    std::vector<AssimpSceneExporter::ExportFormat> formats;
    std::string exportFolder = "";
    bool copyTextures = true;
    bool embedTextures = true;

    void open(const std::string& folder) {
        show = true;
        exportFolder = folder;
        formats = AssimpSceneExporter::getSupportedFormats();
        selectedFormatIndex = 0;
        embedTextures = true;
        copyTextures = true;
    }

    void reset() {
        show = false;
        selectedFormatIndex = 0;
        exportFolder = "";
        copyTextures = true;
        embedTextures = true;
    }
};

static ExportFormatDialog exportFormatDialog;

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

static bool ProjectToScreen(const glm::vec3& p, const glm::mat4& view, const glm::mat4& proj, ImVec2& outScreen) {
    glm::vec4 clip = proj * view * glm::vec4(p, 1.0f);
    if (clip.w <= 0.0f) return false;
    glm::vec3 ndc = glm::vec3(clip) / clip.w; // [-1,1]
    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f || ndc.z < -1.0f || ndc.z > 1.0f) {
        return false;
    }
    float x = (ndc.x * 0.5f + 0.5f) * (float)gFramebufferWidth;
    float y = ((-ndc.y) * 0.5f + 0.5f) * (float)gFramebufferHeight; // inverti Y
    outScreen = ImVec2(x, y);
    return true;
}

static void drawLightDirectionOverlay(const glm::mat4& view, const glm::mat4& projection) {
    if (!gDirLight.enabled) return;

    // Direzione luce (target o Euler)
    glm::vec3 dir = gDirLight.useTarget
        ? glm::normalize(gDirLight.target - gDirLight.position)
        : [&] {
        glm::vec3 e = glm::radians(gDirLight.orientationEuler);
        glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), e.y, glm::vec3(0, 1, 0));
        glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), e.x, glm::vec3(1, 0, 0));
        return glm::normalize(glm::vec3(rotY * rotX * glm::vec4(0, 0, -1, 0)));
        }();

    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    ImVec2 p0, p1;
    bool ok0 = ProjectToScreen(gDirLight.position, view, projection, p0);

    // Calcola il punto di intersezione con il piano y = gFloorHeight
    glm::vec3 end;
    bool hasIntersection = false;
    {
        float denom = dir.y; // componente Y della direzione
        if (std::abs(denom) > 1e-6f) {
            float tHit = (gFloorHeight - gDirLight.position.y) / denom;
            if (tHit >= 0.0f) {
                end = gDirLight.position + dir * tHit;
                hasIntersection = true;
            }
        }
    }

    // Fallback per stabilire una lunghezza fissa
    if (!hasIntersection) {
        if (gDirLight.useTarget) {
            end = gDirLight.target;
        }
        else {
            end = gDirLight.position + dir * 2.0f;
        }
    }

    bool ok1 = ProjectToScreen(end, view, projection, p1);

    if (ok0 && ok1) {
        dl->AddLine(p0, p1, IM_COL32(255, 140, 0, 220), 2.0f);
    }
}

bool isWithinFiniteSpace(const glm::vec3& p) {
    if (gInfiniteGrid) return true;
    float half = gGridHalfSize + gGridBoundaryMargin;
    return p.x >= -half && p.x <= half && p.z >= -half && p.z <= half;
}

glm::vec3 clampToFiniteSpace(const glm::vec3& p) {
    if (gInfiniteGrid) return p;
    float half = gGridHalfSize;
    return glm::vec3(
        std::clamp(p.x, -half, half),
        p.y,
        std::clamp(p.z, -half, half)
    );
}

static float getObjectEffectiveRadius(const SceneObject& obj) {
    // Dimensione base (i modelli cube/sphere/pyramid sembrano unit size → lato 1 → raggio 0.5)
    float baseRadius = 0.5f;
    glm::vec3 s = obj.getScale();
    // Approssimazione: raggio = baseRadius * max(scale.x, scale.z)
    return baseRadius * (std::max)(s.x, s.z);
}

// Target directional light
static constexpr unsigned int gDirLightTargetObjectNumericId = 0x00FFFFFD;
static constexpr float kTargetLift = 0.002f;

static inline glm::vec3 ClampTargetOnFloor(const glm::vec3& p) {
    glm::vec3 c = p;
    c.y = gFloorHeight + kTargetLift;
    return clampToFiniteSpace(c);
}

// Controlla se una posizione è occupata
bool isPositionOccupied(const glm::vec3& position, float newObjRadius, const SceneManager& manager) {
    for (const auto& obj : manager.getObjects()) {
        // Ignora oggetti di sistema
        if (manager.isSystemId(obj->getId()) && obj->getId() == gDirLightTargetObjectNumericId) continue;

        float r = getObjectEffectiveRadius(*obj);
        float combined = r + newObjRadius;
        glm::vec2 a(obj->getPosition().x, obj->getPosition().z);
        glm::vec2 b(position.x, position.z);
        if (glm::distance(a, b) < combined * 0.95f) {
            return true;
        }
    }
    return false;
}

// Calcola la posizione di spawn, gestendo le collisioni
glm::vec3 calculateSpawnPosition(const std::string& modelName, const glm::vec3& cameraTarget, const SceneManager& manager) {
    
    // Offset verticale per tipo modello
    float yOffset = 0.0f;
    if (modelName == "Cube" || modelName == "Sphere") yOffset = 0.5f;
    else if (modelName == "Pyramid") yOffset = 0.0f;
    else yOffset = 1.0f;

    // Raggio presunto del nuovo oggetto
    float newObjRadius = 0.75f;

    // Modalità infinita: usa già la logica interna del SceneManager
    if (gInfiniteGrid) {
        return manager.findValidSpawnPosition(mouseControl.camPos, cameraTarget, yOffset, newObjRadius);
    }

    float halfExtent = gGridHalfSize;
    return manager.findValidSpawnPositionFinite( mouseControl.camPos, cameraTarget, yOffset, newObjRadius, halfExtent, gGridBoundaryMargin);

    // Base centrale (target proiettato sul piano + offset Y)
    glm::vec3 base = cameraTarget;
    base.y = yOffset;
    glm::vec3 clampedCenter = clampToFiniteSpace(base);

    // Se il centro è libero, usa subito
    if (!isPositionOccupied(clampedCenter, newObjRadius, manager)) {
        return clampedCenter;
    }

    // Parametri ricerca
    const int maxRings = 12;
    float cellSize = (newObjRadius * 2.0f) + 0.15f;  // Diametro + padding
    glm::vec3 best = clampedCenter;
    bool found = false;

    for (int ring = 1; ring <= maxRings && !found; ++ring) {
        // Genera offset lungo il perimetro dell'anello corrente
        for (int dx = -ring; dx <= ring && !found; ++dx) {
            // Bordo superiore
            int dzTop = -ring;
            glm::vec3 candidateTop = clampedCenter + glm::vec3(dx * cellSize, 0.0f, dzTop * cellSize);
            candidateTop = clampToFiniteSpace(candidateTop);
            if (!isPositionOccupied(candidateTop, newObjRadius, manager)) {
                best = candidateTop; found = true; break;
            }
            // Bordo inferiore
            int dzBottom = ring;
            if (dzBottom != dzTop) {
                glm::vec3 candidateBottom = clampedCenter + glm::vec3(dx * cellSize, 0.0f, dzBottom * cellSize);
                candidateBottom = clampToFiniteSpace(candidateBottom);
                if (!isPositionOccupied(candidateBottom, newObjRadius, manager)) {
                    best = candidateBottom; found = true; break;
                }
            }
        }
        // Lati verticali
        for (int dz = -ring + 1; dz <= ring - 1 && !found; ++dz) {
            int dxLeft = -ring;
            glm::vec3 candidateLeft = clampedCenter + glm::vec3(dxLeft * cellSize, 0.0f, dz * cellSize);
            candidateLeft = clampToFiniteSpace(candidateLeft);
            if (!isPositionOccupied(candidateLeft, newObjRadius, manager)) {
                best = candidateLeft; found = true; break;
            }
            int dxRight = ring;
            glm::vec3 candidateRight = clampedCenter + glm::vec3(dxRight * cellSize, 0.0f, dz * cellSize);
            candidateRight = clampToFiniteSpace(candidateRight);
            if (!isPositionOccupied(candidateRight, newObjRadius, manager)) {
                best = candidateRight; found = true; break;
            }
        }
    }

    // Se non trovato nulla libera, ritorna il centro clamped della griglia
    return best;
}

// Helper per recuperare oggetto per ID
static std::shared_ptr<SceneObject> findObjectById(unsigned int id) {
    for (const auto& o : sceneManager.getObjects()) {
        if (o->getId() == id) return o;
    }
    return nullptr;
}

// Modifica callback framebuffer
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (width > 0 && height > 0) {
        // Aggiorna dimensioni framebuffer
        gFramebufferWidth = width;
        gFramebufferHeight = height;

        // Aggiorna dimensioni logiche finestra
        int ww, wh;
        glfwGetWindowSize(window, &ww, &wh);
        gWindowWidthLogical = ww;
        gWindowHeightLogical = wh;

        std::cout << "[WINDOW] Resize: Logical(" << gWindowWidthLogical << "x" << gWindowHeightLogical
            << ") Framebuffer(" << gFramebufferWidth << "x" << gFramebufferHeight << ")" << std::endl;

        glViewport(0, 0, gFramebufferWidth, gFramebufferHeight);

        if (pickingBuffer.isInitialized()) {
            pickingBuffer.resize(gFramebufferWidth, gFramebufferHeight);
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
    if (mouseControl.camPos.y < 0.5f) mouseControl.camPos.y = 0.5f;
}

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

// Funzione di supporto per convertire coordinate mouse -> framebuffer + inversione Y
static inline void ConvertCursorToFramebuffer(GLFWwindow* window, double cursorX, double cursorY, int& outX, int& outY) {
    int ww = gWindowWidthLogical;
    int wh = gWindowHeightLogical;
    int fbw = gFramebufferWidth;
    int fbh = gFramebufferHeight;

    // Scala
    double scaledX = cursorX * (double)fbw / (double)ww;
    double scaledY = cursorY * (double)fbh / (double)wh;

    outX = (int)std::clamp(scaledX, 0.0, (double)fbw - 1.0);
    outY = (int)std::clamp(scaledY, 0.0, (double)fbh - 1.0);
}

// Aggiorna processMousePicking per usare le dimensioni corrette
void processMousePicking(int xWindow, int yWindow) {
    if (!pickingEnabled || !pickingBuffer.isInitialized()) return;

    int px, py;
    ConvertCursorToFramebuffer(glfwGetCurrentContext(),
        (double)xWindow, (double)yWindow, px, py);

    // Salva viewport corrente
    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);

    // Stato pulito per pass di picking
    glDisable(GL_BLEND);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);

    pickingBuffer.bind();
    pickingBuffer.clear();

    // Usa l'aspect del picking buffer (più robusto durante resize/HiDPI)
    float aspect = static_cast<float>(pickingBuffer.getWidth()) / static_cast<float>(pickingBuffer.getHeight());
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(mouseControl.camPos, mouseControl.cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    glUseProgram(pickingShader);
    SetUniformMat4(pickingShader, "view", view);
    SetUniformMat4(pickingShader, "projection", projection);

    sceneManager.renderForPicking(pickingShader, view, projection);

    // Sincronizza la scrittura sul FBO prima della lettura
    glFlush();

    glm::vec3 idColor = pickingBuffer.readPixel(px, py);
    pickingBuffer.unbind();

    // Ripristina il viewport originale
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);

    // Quantizza il colore per evitare errori di floating
    glm::vec3 q = QuantizeIdColor(idColor);

    handlePickingResult(q);

#ifdef DEBUG_PICKING
    std::cout << "=== PICKING DEBUG ===" << std::endl;
    std::cout << "Window coords: (" << xWindow << "," << yWindow << ")" << std::endl;
    std::cout << "FB coords: (" << px << "," << py << ")" << std::endl;
    std::cout << "Raw ID Color: " << idColor.r << "," << idColor.g << "," << idColor.b << std::endl;
    std::cout << "Quantized:     " << q.r << "," << q.g << "," << q.b << std::endl;
    std::cout << "Decoded ID:    " << sceneManager.colorToId(q) << std::endl;
#endif
}

float lastClickTime = 0.0f;
const float doubleClickTimeThreshold = 0.3f;
unsigned int lastClickedObjectId = 0;

// Aggiungi queste variabili globali
bool pinSelectedModelPanel = false;
ImVec2 selectedPanelSize = ImVec2(350, 0);

void handlePickingResult(const glm::vec3& idColor) {
    unsigned int pickedId = sceneManager.colorToId(idColor);

    // Click nel vuoto → deseleziona sempre
    if (pickedId == 0) {
        sceneManager.deselectAll();
        objectSelected = false;
        if (!pinSelectedModelPanel) showSelectedModelPanel = false;
        ImGuizmo::Enable(false);
        return;
    }

    if (pickedId == gDirLightTargetObjectNumericId) {
        sceneManager.selectObject(pickedId);
        objectSelected = false; 
        showSelectedModelPanel = false;
        ImGuizmo::Enable(true);
        return;
    }

    sceneManager.selectObject(pickedId);

    bool isLight = (pickedId == gDirLightObjectNumericId);
    objectSelected = !isLight;
    showSelectedModelPanel = !isLight;
    ImGuizmo::Enable(true);

    if (auto sel = sceneManager.getSelectedObject()) {
        displayedEulerAngles = glm::degrees(glm::eulerAngles(sel->getRotation()));
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (objectSelected && (ImGuizmo::IsUsing() || ImGuizmo::IsOver())) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
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
            if (pickingEnabled) {
                processMousePicking((int)xpos, (int)ypos);
            }
            mouseControl.isOrbiting = false;
            mouseControl.isPanning = false;
            mouseControl.isPressed = true;
        }

        mouseControl.lastX = (float)xpos;
        mouseControl.lastY = (float)ypos;
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        mouseControl.isPressed = false;
        mouseControl.isOrbiting = false;
        mouseControl.isPanning = false;
        objectMoving = false;
        currentMoveAxis = MoveAxis::NONE;
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
        if (mouseControl.camPos.y <= 0.5f && yoffset < 0.0f) {
            yoffset = 0.0f;
        }

        float proposedOrbitalY = mouseControl.orbitalAngleY + yoffset * sensitivity;

        if (!(mouseControl.camPos.y <= 0.5f && proposedOrbitalY < mouseControl.orbitalAngleY)) {
            mouseControl.orbitalAngleY = proposedOrbitalY;
            if (mouseControl.orbitalAngleY > 85.0f) mouseControl.orbitalAngleY = 85.0f;
            if (mouseControl.orbitalAngleY < -85.0f) mouseControl.orbitalAngleY = -85.0f;
        }

        // Aggiorna sempre l’angolo orizzontale
        mouseControl.orbitalAngleX += xoffset * sensitivity;

        updateCameraPosition();
    }
    else if (mouseControl.isPanning) {
        const float panSpeed = 0.003f * mouseControl.cameraDistance;

        // Calcola la direzione di vista della camera e proiettala sul piano XZ
        glm::vec3 viewDir = mouseControl.cameraTarget - mouseControl.camPos;
        glm::vec3 forwardOnPlane = glm::normalize(glm::vec3(viewDir.x, 0.0f, viewDir.z));
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
    if (!pickingBuffer.initialize(gFramebufferWidth, gFramebufferHeight)) {
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

void renderScene(GLuint shader, const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(shader);
    SetUniformMat4(shader, "view", view);
    SetUniformMat4(shader, "projection", projection);
    SetUniformVec3(shader, "viewPos", mouseControl.camPos);
    SetUniformFloat(shader, "time", gTime);

    glm::vec3 lightDir;
    if (gDirLight.useTarget) {
        lightDir = glm::normalize(gDirLight.target - gDirLight.position);
    }
    else {
        glm::vec3 e = glm::radians(gDirLight.orientationEuler);
        glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), e.y, glm::vec3(0, 1, 0));
        glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), e.x, glm::vec3(1, 0, 0));
        glm::vec3 forward = glm::vec3(rotY * rotX * glm::vec4(0, 0, -1, 0));
        lightDir = glm::normalize(forward);
    }
    SetUniformInt(shader,  "uDirLight.enabled", gDirLight.enabled ? 1 : 0);
    SetUniformVec3(shader, "uDirLight.direction", lightDir);
    SetUniformVec3(shader, "uDirLight.color", gDirLight.color);

    gShadows.bindForShading(shader, 1, (gEnableShadowMap && gDirLight.enabled));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::vector<std::shared_ptr<SceneObject>> list = sceneManager.getObjects();
    std::stable_sort(list.begin(), list.end(), [&](const auto& a, const auto& b){
        bool aSys = sceneManager.isSystemId(a->getId());
        bool bSys = sceneManager.isSystemId(b->getId());
        if (aSys != bSys) return aSys;
        return a->getId() < b->getId();
    });

    for (const auto& obj : list) {
        glm::mat4 modelMatrix = obj->getModelMatrix();
        SetUniformMat4(shader, "model", modelMatrix);

        bool isLightIcon = (obj->getId() == gDirLightObjectNumericId);
        SetUniformInt(shader, "uLightIcon", isLightIcon ? 1 : 0);
        SetUniformInt(shader, "uLightGradientEnabled", (isLightIcon && gDirLight.useGradient) ? 1 : 0);
        SetUniformVec3(shader, "uLightGradientStart", gDirLight.gradientStart);
        SetUniformVec3(shader, "uLightGradientEnd", gDirLight.gradientEnd);
        SetUniformVec3(shader, "uLightEmitColor", gDirLight.color);
        SetUniformInt(shader, "uUnlit", 0);

        if (isLightIcon) {
            SetUniformInt(shader, "useOverrideColor", 0);
            SetUniformInt(shader, "useTexture", 0);
            obj->getModel()->render();
            continue;
        }

        glActiveTexture(GL_TEXTURE0);
        SetUniformInt(shader, "textureSampler", 0);

        std::shared_ptr<Model> model = obj->getModel();
        if (obj->hasOverrideTexture()) {
            glBindTexture(GL_TEXTURE_2D, obj->getOverrideTextureID());
            SetUniformInt(shader, "useOverrideColor", 0);
            SetUniformInt(shader, "useTexture", 1);
        } else if (obj->hasOverrideColor()) {
            glBindTexture(GL_TEXTURE_2D, TextureManager::getInstance().getDefaultTexture());
            SetUniformInt(shader, "useOverrideColor", 1);
            SetUniformVec4(shader, "overrideColor", obj->getOverrideColor());
            SetUniformInt(shader, "useTexture", 0);
        }
        else if (model->hasTexture()) {
            glBindTexture(GL_TEXTURE_2D, model->getTextureID());
            SetUniformInt(shader, "useOverrideColor", 0);
            SetUniformInt(shader, "useTexture", 1);
        }
        else {
            glBindTexture(GL_TEXTURE_2D, TextureManager::getInstance().getDefaultTexture());
            SetUniformInt(shader, "useOverrideColor", 0);
            SetUniformInt(shader, "useTexture", 0);
        }
        model->render();
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#endif
#include <stb_image.h>
std::string openFolderDialog() {
#ifdef _WIN32
    std::string selectedPath;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileDialog* pfd = nullptr;
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&pfd));

        if (SUCCEEDED(hr)) {
            DWORD dwOptions;
            pfd->GetOptions(&dwOptions);
            pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
            pfd->SetTitle(L"Scegli dove salvare la scena");

            hr = pfd->Show(NULL);
            if (SUCCEEDED(hr)) {
                IShellItem* psi;
                hr = pfd->GetResult(&psi);
                if (SUCCEEDED(hr)) {
                    PWSTR pszPath;
                    hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                    if (SUCCEEDED(hr)) {
                        // Converti da WCHAR* a std::string
                        int size = WideCharToMultiByte(CP_UTF8, 0, pszPath, -1,
                            nullptr, 0, nullptr, nullptr);
                        selectedPath.resize(size - 1);
                        WideCharToMultiByte(CP_UTF8, 0, pszPath, -1,
                            &selectedPath[0], size, nullptr, nullptr);
                        CoTaskMemFree(pszPath);
                    }
                    psi->Release();
                }
            }
            pfd->Release();
        }
        CoUninitialize();
    }

    return selectedPath;
#else
    return "";
#endif
}

void renderSceneControlPanel() {
    if (ImGui::CollapsingHeader("Scene Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Scene Management:");

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.3f, 1.0f));

        if (ImGui::Button("Export Scene Bundle", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) {
            std::string folderPath = openFolderDialog();
            if (!folderPath.empty()) {
                exportFormatDialog.open(folderPath);
            }
            else {
                std::cout << "Export cancellato dall'utente" << std::endl;
            }
        }

        ImGui::PopStyleColor(3);

        ImGui::Separator();
        ImGui::Text("Add / Import:");

        // Pulsante Load 3D Model (disabilitato se limite raggiunto)
        {
            bool disabled = gFiniteLimitReached && !gInfiniteGrid;
            if (disabled) ImGui::BeginDisabled();

            ImGui::PushStyleColor(ImGuiCol_Button,
                                  disabled ? ImVec4(0.35f, 0.35f, 0.35f, 1.0f)
                                           : ImVec4(0.2f, 0.6f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  disabled ? ImVec4(0.45f, 0.45f, 0.45f, 1.0f)
                                           : ImVec4(0.3f, 0.7f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                  disabled ? ImVec4(0.25f, 0.25f, 0.25f, 1.0f)
                                           : ImVec4(0.1f, 0.5f, 0.3f, 1.0f));

            if (ImGui::Button("Load 3D Model", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))
                && !disabled) {
                ImGui::OpenPopup("Choose Load Mode");
            }

            ImGui::PopStyleColor(3);
            if (disabled) {
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Limite raggiunto in modalita' finita.");
                }
                ImGui::EndDisabled();
            }
        }

        // Pulsante Load Image (anche lui limitato)
        {
            bool disabled = gFiniteLimitReached && !gInfiniteGrid;
            if (disabled) ImGui::BeginDisabled();

            ImGui::PushStyleColor(ImGuiCol_Button,
                                  disabled ? ImVec4(0.35f, 0.35f, 0.35f, 1.0f)
                                           : ImVec4(0.2f, 0.6f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  disabled ? ImVec4(0.45f, 0.45f, 0.45f, 1.0f)
                                           : ImVec4(0.3f, 0.7f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                  disabled ? ImVec4(0.25f, 0.25f, 0.25f, 1.0f)
                                           : ImVec4(0.1f, 0.5f, 0.3f, 1.0f));

            if (ImGui::Button("Load Image", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))
                && !disabled) {
                openImageFile();
            }

            ImGui::PopStyleColor(3);
            if (disabled) {
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Limite raggiunto in modalita' finita.");
                }
                ImGui::EndDisabled();
            }
        }

        // Popup modalità caricamento (resta invariato salvo condizione di apertura già gestita sopra)
        if (ImGui::BeginPopupModal("Choose Load Mode", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("How do you want to load this model?");
            ImGui::Separator();

            if (ImGui::Button("Single Object (Default)", ImVec2(250, 0))) {
                ModelLoader::openModelFileAdvanced(
                    modelManager, sceneManager,
                    mouseControl.camPos, mouseControl.cameraTarget,
                    objectSelected, showSelectedModelPanel,
                    ModelLoader::LoadMode::SINGLE_OBJECT);
                ImGui::CloseCurrentPopup();
            }
            ImGui::TextWrapped("   All meshes in one selectable object");

            ImGui::Spacing();

            if (ImGui::Button("Separate Meshes", ImVec2(250, 0))) {
                ModelLoader::openModelFileAdvanced(
                    modelManager, sceneManager,
                    mouseControl.camPos, mouseControl.cameraTarget,
                    objectSelected, showSelectedModelPanel,
                    ModelLoader::LoadMode::SEPARATE_MESHES);
                ImGui::CloseCurrentPopup();
            }
            ImGui::TextWrapped("   Each mesh becomes a separate object");

            ImGui::Separator();

            if (ImGui::Button("Cancel", ImVec2(250, 0))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::Separator();
        ImGui::Text("Add Built-in Object:");

        std::vector<std::string> modelNames = modelManager.getModelNames();
        modelNames.erase(std::remove(modelNames.begin(), modelNames.end(), "ImagePlane"), modelNames.end());
        static int selectedModelIndex = 0;

        if (!modelNames.empty()) {
            ImGui::Combo("Model Type", &selectedModelIndex,
                         [](void* data, int idx, const char** out_text) {
                             auto& names = *static_cast<std::vector<std::string>*>(data);
                             if (idx >= 0 && idx < (int)names.size()) {
                                 *out_text = names[idx].c_str();
                                 return true;
                             }
                             return false;
                         },
                         &modelNames,
                         (int)modelNames.size());

            bool disabled = (!gInfiniteGrid && gFiniteLimitReached);

            if (disabled) ImGui::BeginDisabled();

            ImGui::PushStyleColor(ImGuiCol_Button,
                                  disabled ? ImVec4(0.35f, 0.35f, 0.35f, 1.0f)
                                           : ImVec4(0.2f, 0.6f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  disabled ? ImVec4(0.45f, 0.45f, 0.45f, 1.0f)
                                           : ImVec4(0.3f, 0.7f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                  disabled ? ImVec4(0.25f, 0.25f, 0.25f, 1.0f)
                                           : ImVec4(0.1f, 0.5f, 0.3f, 1.0f));

            if (ImGui::Button("Add Object", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))
                && !disabled && selectedModelIndex < (int)modelNames.size()) {

                const std::string& modelName = modelNames[selectedModelIndex];
                glm::vec3 spawnPos = calculateSpawnPosition(modelName, mouseControl.cameraTarget, sceneManager);

                if (!gInfiniteGrid) {
                    glm::vec3 clamped = clampToFiniteSpace(spawnPos);
                    if (clamped != spawnPos) {
                        gLastPlacementClamped = true;
                        gBoundaryWarningMessage = "Posizione fuori area: adattata ai confini.";
                        spawnPos = clamped;
                    } else {
                        gLastPlacementClamped = false;
                    }
                }

                auto newObj = sceneManager.addObject(modelName, spawnPos);
                if (newObj) {
                    sceneManager.applyDefaultName(newObj->getId(), modelName);
                    sceneManager.selectObject(newObj->getId());
                    objectSelected = true;
                    showSelectedModelPanel = true;
                }
            }

            ImGui::PopStyleColor(3);

            if (disabled) {
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Limite massimo (%d) raggiunto.", gFiniteModeMaxObjects);
                }
                ImGui::EndDisabled();
            }
        }

        ImGui::Separator();


        // Variabili per gestire la rinomina
        static bool isRenaming = false;
        static unsigned int renamingId = 0;
        static char renameBuffer[128] = "";

        std::vector<std::shared_ptr<SceneObject>> userObjects;
        std::vector<std::shared_ptr<SceneObject>> systemObjects;

        for (const auto& obj : sceneManager.getObjects()) {
            if (sceneManager.isSystemId(obj->getId())) {
                systemObjects.push_back(obj);
            }
            else {
                userObjects.push_back(obj);
            }
        }

        ImGui::Text("User Objects (%zu/%d):", userObjects.size(), gFiniteModeMaxObjects);

        // Sezione oggetti utente
        for (const auto& obj : userObjects) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (obj->getSelected()) flags |= ImGuiTreeNodeFlags_Selected;

            bool renamingThis = isRenaming && renamingId == obj->getId();
            if (renamingThis) {
                ImGui::PushID(static_cast<int>(obj->getId()));
                if (ImGui::InputText("##rename_user", renameBuffer, sizeof(renameBuffer),
                    ImGuiInputTextFlags_EnterReturnsTrue)) {
                    sceneManager.renameObject(obj->getId(), renameBuffer);
                    isRenaming = false;
                }
                if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) {
                    isRenaming = false;
                }
                ImGui::PopID();
            }
            else {
                ImGui::TreeNodeEx(obj->getName().c_str(), flags);
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    isRenaming = true;
                    renamingId = obj->getId();
                    strncpy(renameBuffer, obj->getName().c_str(), sizeof(renameBuffer) - 1);
                    renameBuffer[sizeof(renameBuffer) - 1] = '\0';
                }
            }

            if (!renamingThis && ImGui::IsItemClicked()) {
                sceneManager.selectObject(obj->getId());
                objectSelected = true;
                showSelectedModelPanel = true;
                showDirectionalLightWindow = false;
            }

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Rename")) {
                    isRenaming = true;
                    renamingId = obj->getId();
                    strncpy(renameBuffer, obj->getName().c_str(), sizeof(renameBuffer) - 1);
                    renameBuffer[sizeof(renameBuffer) - 1] = '\0';
                }
                if (ImGui::MenuItem("Delete")) {
                    unsigned int idToRemove = obj->getId();
                    if (obj->getSelected()) {
                        objectSelected = false;
                        showSelectedModelPanel = false;
                    }
                    sceneManager.removeObject(idToRemove);
                    ImGui::EndPopup();
                    break;
                }
                ImGui::EndPopup();
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("System Objects (%zu):", systemObjects.size());

        // Sezione oggetti di sistema (solo selezione / rename opzionale luce)
        for (const auto& obj : systemObjects) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (obj->getSelected()) flags |= ImGuiTreeNodeFlags_Selected;

            bool renamingThis = isRenaming && renamingId == obj->getId();
            bool isLight = (obj->getId() == gDirLightObjectNumericId);
            bool isLightTarget = (obj->getId() == gDirLightTargetObjectNumericId);

            if (renamingThis && isLight) {
                ImGui::PushID(static_cast<int>(obj->getId()));
                if (ImGui::InputText("##rename_sys", renameBuffer, sizeof(renameBuffer),
                    ImGuiInputTextFlags_EnterReturnsTrue)) {
                    sceneManager.renameObject(obj->getId(), renameBuffer);
                    isRenaming = false;
                }
                if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) {
                    isRenaming = false;
                }
                ImGui::PopID();
            }
            else {
                ImGui::TreeNodeEx(obj->getName().c_str(), flags);
                if (isLight && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    isRenaming = true;
                    renamingId = obj->getId();
                    strncpy(renameBuffer, obj->getName().c_str(), sizeof(renameBuffer) - 1);
                    renameBuffer[sizeof(renameBuffer) - 1] = '\0';
                }
            }

            if (!renamingThis && ImGui::IsItemClicked()) {
                sceneManager.selectObject(obj->getId());
                objectSelected = (!isLight);
                showSelectedModelPanel = !isLight;
                if (isLight && !pinDirectionalLightWindow) {
                    showDirectionalLightWindow = true;
                }
            }

            if (ImGui::BeginPopupContextItem()) {
                if ((isLight || isLightTarget) && ImGui::MenuItem("Rename")) {
                    isRenaming = true;
                    renamingId = obj->getId();
                    strncpy(renameBuffer, obj->getName().c_str(), sizeof(renameBuffer) - 1);
                    renameBuffer[sizeof(renameBuffer) - 1] = '\0';
                }
                ImGui::TextDisabled("System object");
                ImGui::EndPopup();
            }
        }

        ImGui::Separator();
        ImGui::Checkbox("Enable Picking", &pickingEnabled);
    }
}

// Dialog informativo limite oggetti in modalità finita
void renderFiniteLimitDialog() {
    if (!showFiniteLimitDialog || !gFiniteLimitReached) return;

    ImGui::SetNextWindowSize(ImVec2(360, 150), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20 + SIDEBAR_WIDTH, 20), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Limite oggetti raggiunto", &showFiniteLimitDialog, flags)) {
        ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.25f, 1.0f),
            "Limite massimo di %d oggetti raggiunto.", gFiniteModeMaxObjects);
        ImGui::Separator();
        ImGui::TextWrapped("Passa a 'Infinita' nella sezione Grid Mode per rimuovere questo limite.");
        ImGui::Spacing();
        if (ImGui::Button("Nascondi", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            showFiniteLimitDialog = false;
        }
    }
    ImGui::End();
}

void openTextureFile() {
    std::string texturePath;
    GLuint textureID;
    bool needsConfirmation = false;

    if (ModelLoader::openTextureFile(modelManager, sceneManager,
        texturePath, textureID, needsConfirmation)) {
        if (needsConfirmation && !dontAskTextureReplace) {
            // Mostra dialogo di conferma
            textureReplaceDialog.show = true;
            textureReplaceDialog.texturePath = texturePath;
            textureReplaceDialog.newTextureID = textureID;
        }
        else if (needsConfirmation && dontAskTextureReplace) {
            // Applica la texture E pulisci il colore
            if (auto selectedObj = sceneManager.getSelectedObject()) {
                selectedObj->setOverrideTexture(textureID);
            }
            if (auto selectedObj = sceneManager.getSelectedObject()) {
                selectedObj->clearOverrideColor();
            }
        }
    }
    else {
        if (!texturePath.empty()) {
            errorDialog.show = true;
            errorDialog.title = "Texture Loading Error";
            errorDialog.message = "Unable to load the texture from the selected file.";
        }
    }
}

bool shouldBlockAdd() {
    return (!gInfiniteGrid && gFiniteLimitReached);
}

void openModelFile() {
    if (shouldBlockAdd()) {
        showFiniteLimitDialog = true;
        return;
    }
    // Timing preciso per loading dialog
    auto loadStartTime = std::chrono::steady_clock::now();
    bool dialogShown = false;
    
    glm::vec3 spawnPos = sceneManager.findValidSpawnPosition(
        mouseControl.camPos, 
        mouseControl.cameraTarget, 
        0.5f, 1.0f 
    );
    
    // Prepara il callback per progress updates CON TIMING
    auto progressCallback = [&dialogShown, &loadStartTime](float progress, const std::string& message) {
        auto elapsed = std::chrono::steady_clock::now() - loadStartTime;
        auto elapsedSeconds = std::chrono::duration<float>(elapsed).count();
        
        // Mostra dialog solo se passano più di 2 secondi
        if (elapsedSeconds > 2.0f && !dialogShown) {
            loadingDialog.start("Loading model...");
            dialogShown = true;
        }
        
        if (dialogShown) {
            loadingDialog.updateProgress(progress, message);
            
            // Forza rendering durante il caricamento
            glfwPollEvents();
            
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            renderLoadingDialog();
            
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(glfwGetCurrentContext());
        }
    };
    
    bool success = ModelLoader::openModelFileAdvanced(
        modelManager, sceneManager,
        mouseControl.camPos, mouseControl.cameraTarget,
        objectSelected, showSelectedModelPanel,
        ModelLoader::LoadMode::SEPARATE_MESHES,
        progressCallback
    );
    
    auto totalElapsed = std::chrono::steady_clock::now() - loadStartTime;
    auto totalSeconds = std::chrono::duration<float>(totalElapsed).count();
    
    if (success) {
        if (dialogShown) {

            std::string successMessage = "Model loaded successfully!\n\n";
            successMessage += "Time: " + std::to_string(totalSeconds) + " seconds\n";
            // Dettagli mesh e texture
            auto& objects = sceneManager.getObjects();
            if (!objects.empty()) {
                int meshCount = 0;
                int textureCount = 0;
                
                for (const auto& obj : objects) {
                    if (obj->getModel()) {
                        meshCount++;
                        if (obj->getModel()->hasTexture()) {
                            textureCount++;
                        }
                    }
                }
                
                successMessage += "Meshes: " + std::to_string(meshCount) + "\n";
                successMessage += "Textures: " + std::to_string(textureCount) + "\n";
            }
            
            loadingDialog.complete();
            loadingDialog.statusMessage = successMessage;
            
            // Auto-chiudi dopo 3 secondi
            std::thread([]{
                std::this_thread::sleep_for(std::chrono::milliseconds(3000));
                loadingDialog.reset();
            }).detach();
        }
    } else {
        if (dialogShown) {
            loadingDialog.error("Failed to load model");
        }
    }
}

void openImageFile() {
    if (shouldBlockAdd()) {
        showFiniteLimitDialog = true;
        return;
    }
    // Calcola la posizione di spawn valida
    glm::vec3 spawnPos = sceneManager.findValidSpawnPosition(
        mouseControl.camPos, 
        mouseControl.cameraTarget, 
        0.5f,  // offset Y
        1.0f   // raggio collisione
    );
    
    std::string errorMessage;
    
    bool success = ModelLoader::openImageFile(
        modelManager, 
        sceneManager,
        mouseControl.camPos, 
        spawnPos,
        objectSelected, 
        showSelectedModelPanel, 
        errorMessage
    );
    
    // Gestione errori
    if (!success && !errorMessage.empty()) {
        errorDialog.show = true;
        errorDialog.title = "Image Loading Error";
        errorDialog.message = errorMessage;
    }
}

void renderExportFormatDialog() {
    if (!exportFormatDialog.show) return;
    
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_Always);
    
    ImGui::OpenPopup("Export 3D Scene");
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.6f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.3f, 0.7f, 0.5f, 1.0f));
    
    if (ImGui::BeginPopupModal("Export 3D Scene", &exportFormatDialog.show, 
                               ImGuiWindowFlags_NoResize)) {
        
        ImGui::TextWrapped("Scegli il formato di export per la tua scena 3D.");
        ImGui::Separator();
        ImGui::Spacing();
        
        // Lista formati con radio buttons
        ImGui::Text("Formato File:");
        ImGui::Spacing();
        
        for (size_t i = 0; i < exportFormatDialog.formats.size(); i++) {
            const auto& fmt = exportFormatDialog.formats[i];
            
            bool isSelected = (i == exportFormatDialog.selectedFormatIndex);
            
            if (ImGui::RadioButton(fmt.name.c_str(), isSelected)) {
                exportFormatDialog.selectedFormatIndex = static_cast<int>(i);
            }
            
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", fmt.extension.c_str());
            
            // Descrizione del formato
            ImGui::Indent(30);
            ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x - 10);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", fmt.description.c_str());
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "→ %s", fmt.useCases.c_str());
            ImGui::PopTextWrapPos();
            ImGui::Unindent(30);
            
            ImGui::Spacing();
        }
        
        ImGui::Separator();
        
        // Opzioni aggiuntive
        ImGui::Spacing();
        ImGui::Text("Opzioni:");

        // Opzione embedding (se supportato)
        if (exportFormatDialog.selectedFormatIndex < exportFormatDialog.formats.size()) {
            const auto& selectedFmt = exportFormatDialog.formats[exportFormatDialog.selectedFormatIndex];

            bool canEmbed = selectedFmt.supportsTextures && selectedFmt.supportsEmbeddedTextures;
            bool canCopy  = selectedFmt.supportsTextures;

            // Embed checkbox: disabilitata se non supportata
            if (!canEmbed) ImGui::BeginDisabled();
            ImGui::Checkbox("Incorpora texture nel file (se supportato)", &exportFormatDialog.embedTextures);
            if (!canEmbed) {
                ImGui::EndDisabled();
                exportFormatDialog.embedTextures = false;
                ImGui::TextColored(ImVec4(0.95f, 0.7f, 0.2f, 1.0f),
                    "Il formato selezionato non supporta l'incorporamento. Le texture verranno salvate separatamente.");
            }

            if (!canCopy) ImGui::BeginDisabled();
            bool disableCopy = exportFormatDialog.embedTextures && canEmbed;
            if (disableCopy) ImGui::BeginDisabled();
            ImGui::Checkbox("Copia texture nella cartella export", &exportFormatDialog.copyTextures);
            if (disableCopy) {
                ImGui::EndDisabled();
                exportFormatDialog.copyTextures = false;
                ImGui::TextDisabled("Con incorporamento attivo non è necessaria la copia esterna.");
            }
            if (!canEmbed && canCopy) {
                // formato senza embed: forzo copia
                exportFormatDialog.copyTextures = true;
                ImGui::TextDisabled("Le texture saranno salvate in 'textures/' accanto al file.");
            }
            if (!canCopy) ImGui::EndDisabled();

            bool embed = exportFormatDialog.embedTextures && selectedFmt.supportsEmbeddedTextures && selectedFmt.supportsTextures;
            bool copy = selectedFmt.supportsTextures && exportFormatDialog.copyTextures && !embed;

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Output previsto:");
            ImGui::Indent(20);
            if (embed) {
                ImGui::BulletText("Un singolo file %s con texture incorporate", selectedFmt.extension.c_str());
            }
            else if (copy) {
                ImGui::BulletText("File %s + cartella 'textures/' con immagini referenziate", selectedFmt.extension.c_str());
            }
            else if (selectedFmt.supportsTextures) {
                ImGui::BulletText("File %s senza texture incorporate né cartella textures", selectedFmt.extension.c_str());
            }
            else {
                ImGui::BulletText("File %s (formato senza materiali/texture)", selectedFmt.extension.c_str());
            }
            ImGui::Unindent(20);

            // Dettagli formato selezionato
            ImGui::Text("Dettagli Formato:");
            ImGui::Indent(20);
            ImGui::BulletText("Materiali: %s", selectedFmt.supportsMaterials ? "Sì" : "No");
            ImGui::BulletText("Texture: %s", selectedFmt.supportsTextures ? "Sì" : "No");
            ImGui::BulletText("Texture embedded: %s", selectedFmt.supportsEmbeddedTextures ? "Sì" : "No");
            ImGui::BulletText("Animazioni: %s", selectedFmt.supportsAnimations ? "Sì" : "No");
            ImGui::BulletText("Tipo: %s", selectedFmt.isBinary ? "Binario" : "Testo");
            ImGui::Unindent(20);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Pulsanti azione
        float buttonWidth = (ImGui::GetContentRegionAvail().x - 10) / 2;
        
        if (ImGui::Button("Annulla", ImVec2(buttonWidth, 35))) {
            exportFormatDialog.reset();
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.2f, 1.0f));

        if (ImGui::Button("Esporta", ImVec2(buttonWidth, 35))) {
            const auto& selectedFmt = exportFormatDialog.formats[exportFormatDialog.selectedFormatIndex];

            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");

            std::string filename = "Scene_" + ss.str() + selectedFmt.extension;
            std::string outputPath = exportFormatDialog.exportFolder + "/" + filename;

            std::cout << "[EXPORT] Inizio export in formato: " << selectedFmt.name << std::endl;

            // Determina i flag effettivi
            bool embed = exportFormatDialog.embedTextures && selectedFmt.supportsEmbeddedTextures && selectedFmt.supportsTextures;
            bool copy  = exportFormatDialog.copyTextures && selectedFmt.supportsTextures && !embed;

            auto result = AssimpSceneExporter::exportScene(
                sceneManager,
                modelManager,
                outputPath,
                selectedFmt.id,
                copy,     // copyTextures
                embed     // embedTextures
            );

            if (result.success) {
                successDialog.show = true;
                successDialog.title = "Export Successful";
                successDialog.message = "Scena esportata con successo!";
                
                // Dettagli arricchiti
                std::stringstream details;
                details
                    << "Formato: " << selectedFmt.name << "\n"
                    << "File: " << filename << "\n"
                    << "Vertici: " << result.totalVertices << "\n"
                    << "Facce: " << result.totalFaces << "\n"
                    << "Materiali: " << result.totalMaterials << "\n"
                    << "Dimensione: " << (result.fileSize / 1024) << " KB\n"
                    << "Tempo: " << result.exportTimeSeconds << " s\n";

                if (selectedFmt.supportsTextures) {
                    if (result.texturesEmbedded) {
                        details << "Texture: " << result.texturesEmbeddedCount << " (incorporate)\n";
                    } else if (result.externalTexturesCopied) {
                        details << "Texture: " << result.texturesCopiedCount << " (copiate in /textures)\n";
                    } else {
                        details << "Texture: " << result.totalTextures << "\n";
                    }
                    if (result.texturesMissing > 0) {
                        details << "Texture non salvate: " << result.texturesMissing << "\n";
                    }
                }

                if (!result.warnings.empty()) {
                    details << "\nNote:\n";
                    for (const auto& w : result.warnings) {
                        details << " - " << w << "\n";
                    }
                }
                
                successDialog.details = details.str();
            }
            else {
                errorDialog.show = true;
                errorDialog.title = "Export Error";
                errorDialog.message = result.errorMessage;
            }
            
            exportFormatDialog.reset();
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::PopStyleColor(3);
        
        ImGui::EndPopup();
    }
    
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

// Aggiorna stato limite in modalità finita; apre il dialog quando si raggiunge per la prima volta
void updateFiniteLimitState() {
    static bool previousReached = false;

    if (gInfiniteGrid) {
        gFiniteLimitReached = false;
        showFiniteLimitDialog = false;
        previousReached = false;
        return;
    }

    // Conta solo gli oggetti non di sistema
    size_t nonSystemCount = 0;
    for (const auto& obj : sceneManager.getObjects()) {
        if (sceneManager.isSystemId(obj->getId())) continue;
        nonSystemCount++;
    }

    gFiniteLimitReached = (nonSystemCount >= static_cast<size_t>(gFiniteModeMaxObjects));

    if (gFiniteLimitReached && !previousReached) {
        showFiniteLimitDialog = true;
    }

    previousReached = gFiniteLimitReached;
}

static void renderGridModeSwitchErrorDialog() {
    if (!showGridModeSwitchErrorPopup) return;

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::OpenPopup("Cambio Modalita' Griglia Bloccato");

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22, 18));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.75f, 0.25f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.85f, 0.30f, 0.25f, 1.0f));

    if (ImGui::BeginPopupModal("Cambio Modalita' Griglia Bloccato", &showGridModeSwitchErrorPopup,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {

        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.25f, 1.0f), "IMPOSSIBILE PASSARE ALLA MODALITA' FINITA");
        ImGui::Separator();
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x - 10);
        ImGui::TextWrapped("%s", gGridModeSwitchErrorMessage.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Separator();
        ImGui::TextDisabled("Suggerimento: riduci il numero di oggetti o riposizionali entro i confini.");
        ImGui::Spacing();
        float buttonWidth = 140.0f;
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((avail - buttonWidth) * 0.5f);
        if (ImGui::Button("Chiudi", ImVec2(buttonWidth, 32)) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape) ||
            ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            showGridModeSwitchErrorPopup = false;
            gGridModeSwitchErrorMessage.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

// Separator 
void renderSeparator() {
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.4f, 0.4f, 0.45f, 0.6f));
    ImGui::Separator();
    ImGui::PopStyleColor();
}

// Logging & Console
void renderSectionLoggingConsole() {
    if (ImGui::CollapsingHeader("Logging & Console", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.50f, 0.70f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.60f, 0.80f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.40f, 0.60f, 1.0f));

        if (ImGui::Button("Log Window", ImVec2(-1, 30))) {
            gShowLogger = !gShowLogger;
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.60f, 0.80f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.70f, 0.90f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.50f, 0.70f, 1.0f));

        ImGui::PopStyleColor(6);
    }
}

// Grid Mode
void renderSectionGridMode() {
    if (ImGui::CollapsingHeader("Grid Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
        int currentMode = gInfiniteGrid ? 1 : 0;
        bool clickedFinite = ImGui::RadioButton("Finita", currentMode == 0);
        ImGui::SameLine();
        bool clickedInfinite = ImGui::RadioButton("Infinita", currentMode == 1);

        if (clickedFinite && gInfiniteGrid) {
            std::vector<std::string> reasons;
            int nonSystemCount = 0;
            std::vector<std::string> outOfBoundsNames;
            float half = gGridHalfSize + gGridBoundaryMargin;

            for (const auto& obj : sceneManager.getObjects()) {
                if (sceneManager.isSystemId(obj->getId())) continue;
                nonSystemCount++;
                const glm::vec3 p = obj->getPosition();
                if (p.x < -half || p.x > half || p.z < -half || p.z > half) {
                    outOfBoundsNames.push_back(obj->getName());
                }
            }

            if (nonSystemCount > gFiniteModeMaxObjects) {
                reasons.push_back("Numero di oggetti (" + std::to_string(nonSystemCount) +
                    ") supera il limite massimo (" + std::to_string(gFiniteModeMaxObjects) + ").");
            }

            if (!outOfBoundsNames.empty()) {
                std::string list;
                const size_t maxShow = 8;
                for (size_t i = 0; i < outOfBoundsNames.size(); ++i) {
                    if (i) list += ", ";
                    list += outOfBoundsNames[i];
                    if (i + 1 == maxShow && outOfBoundsNames.size() > maxShow) {
                        list += " ...";
                        break;
                    }
                }
                reasons.push_back("Alcuni oggetti sono fuori dai confini: " + list);
            }

            if (!reasons.empty()) {
                currentMode = 1;
                gInfiniteGrid = true;
                gGridModeSwitchErrorMessage.clear();
                for (size_t i = 0; i < reasons.size(); ++i) {
                    if (i) gGridModeSwitchErrorMessage += "\n\n";
                    gGridModeSwitchErrorMessage += reasons[i];
                }
                showGridModeSwitchErrorPopup = true;
            }
            else {
                currentMode = 0;
                gInfiniteGrid = false;
                sceneManager.setFiniteSpace(true, half, gGridBoundaryMargin);
            }
        }
        else if (clickedInfinite && !gInfiniteGrid) {
            currentMode = 1;
            gInfiniteGrid = true;
            sceneManager.setFiniteSpace(false, 0.0f, 0.0f);
            showFiniteLimitDialog = false;
        }

        gInfiniteGrid = (currentMode == 1);

        if (!gInfiniteGrid) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.65f, 0.15f, 1.0f));
            ImGui::TextWrapped("SPAZIO LIMITATO");
            ImGui::PopStyleColor();

            if (gFiniteLimitReached) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.3f, 0.3f, 1.0f));
                ImGui::TextWrapped("Limite raggiunto");
                ImGui::PopStyleColor();
            }
        }
    }
}

// Scene Objects
void renderSectionSceneObjects() {
    if (ImGui::CollapsingHeader("Scene Objects", ImGuiTreeNodeFlags_DefaultOpen)) {

        // Export Button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.3f, 1.0f));

        if (ImGui::Button("Export Scene", ImVec2(-1, 30))) {
            std::string folderPath = openFolderDialog();
            if (!folderPath.empty()) {
                exportFormatDialog.open(folderPath);
            }
        }
        ImGui::PopStyleColor(3);

        ImGui::Spacing();

        // Add/Import Buttons
        ImGui::Text("Add / Import:");

        bool disabled = gFiniteLimitReached && !gInfiniteGrid;
        if (disabled) ImGui::BeginDisabled();

        ImGui::PushStyleColor(ImGuiCol_Button, disabled ? ImVec4(0.35f, 0.35f, 0.35f, 1.0f) : ImVec4(0.2f, 0.6f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, disabled ? ImVec4(0.45f, 0.45f, 0.45f, 1.0f) : ImVec4(0.3f, 0.7f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, disabled ? ImVec4(0.25f, 0.25f, 0.25f, 1.0f) : ImVec4(0.1f, 0.5f, 0.3f, 1.0f));

        if (ImGui::Button("Load 3D Model", ImVec2(-1, 28)) && !disabled) {
            ImGui::OpenPopup("Choose Load Mode");
        }

        if (ImGui::Button("Load Image", ImVec2(-1, 28)) && !disabled) {
            openImageFile();
        }

        ImGui::PopStyleColor(3);
        if (disabled) ImGui::EndDisabled();

        ImGui::Spacing();

        // Built-in Objects
        ImGui::Text("Add Built-in:");

        std::vector<std::string> modelNames = modelManager.getModelNames();
        modelNames.erase(std::remove(modelNames.begin(), modelNames.end(), "ImagePlane"), modelNames.end());
        static int selectedModelIndex = 0;

        if (!modelNames.empty()) {
            ImGui::PushItemWidth(-1);
            ImGui::Combo("##ModelType", &selectedModelIndex,
                [](void* data, int idx, const char** out_text) {
                    auto& names = *static_cast<std::vector<std::string>*>(data);
                    if (idx >= 0 && idx < (int)names.size()) {
                        *out_text = names[idx].c_str();
                        return true;
                    }
                    return false;
                },
                &modelNames,
                (int)modelNames.size());
            ImGui::PopItemWidth();

            if (disabled) ImGui::BeginDisabled();

            ImGui::PushStyleColor(ImGuiCol_Button, disabled ? ImVec4(0.35f, 0.35f, 0.35f, 1.0f) : ImVec4(0.2f, 0.6f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, disabled ? ImVec4(0.45f, 0.45f, 0.45f, 1.0f) : ImVec4(0.3f, 0.7f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, disabled ? ImVec4(0.25f, 0.25f, 0.25f, 1.0f) : ImVec4(0.1f, 0.5f, 0.3f, 1.0f));

            if (ImGui::Button("Add Object", ImVec2(-1, 28)) && !disabled && selectedModelIndex < (int)modelNames.size()) {
                const std::string& modelName = modelNames[selectedModelIndex];
                glm::vec3 spawnPos = calculateSpawnPosition(modelName, mouseControl.cameraTarget, sceneManager);

                if (!gInfiniteGrid) {
                    glm::vec3 clamped = clampToFiniteSpace(spawnPos);
                    if (clamped != spawnPos) {
                        gLastPlacementClamped = true;
                        spawnPos = clamped;
                    }
                }

                auto newObj = sceneManager.addObject(modelName, spawnPos);
                if (newObj) {
                    sceneManager.applyDefaultName(newObj->getId(), modelName);
                    sceneManager.selectObject(newObj->getId());
                    objectSelected = true;
                    showSelectedModelPanel = true;
                }
            }

            ImGui::PopStyleColor(3);
            if (disabled) ImGui::EndDisabled();
        }

        ImGui::Spacing();

        // Object List
        ImGui::Text("Objects:");

        float listHeight = ImGui::GetContentRegionAvail().y - 60;
        if (listHeight < 100) listHeight = 100;

        ImGui::BeginChild("##ObjectsList", ImVec2(0, listHeight), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        static bool isRenaming = false;
        static unsigned int renamingId = 0;
        static char renameBuffer[128] = "";

        std::vector<std::shared_ptr<SceneObject>> userObjects;
        std::vector<std::shared_ptr<SceneObject>> systemObjects;

        for (const auto& obj : sceneManager.getObjects()) {
            if (sceneManager.isSystemId(obj->getId())) {
                systemObjects.push_back(obj);
            }
            else {
                userObjects.push_back(obj);
            }
        }

        ImGui::Text("User (%zu/%d):", userObjects.size(), gFiniteModeMaxObjects);

        for (const auto& obj : userObjects) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (obj->getSelected()) flags |= ImGuiTreeNodeFlags_Selected;

            bool renamingThis = isRenaming && renamingId == obj->getId();
            if (renamingThis) {
                ImGui::PushID(static_cast<int>(obj->getId()));
                if (ImGui::InputText("##rename", renameBuffer, sizeof(renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    sceneManager.renameObject(obj->getId(), renameBuffer);
                    isRenaming = false;
                }
                if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) {
                    isRenaming = false;
                }
                ImGui::PopID();
            }
            else {
                ImGui::TreeNodeEx(obj->getName().c_str(), flags);
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    isRenaming = true;
                    renamingId = obj->getId();
                    strncpy(renameBuffer, obj->getName().c_str(), sizeof(renameBuffer) - 1);
                    renameBuffer[sizeof(renameBuffer) - 1] = '\0';
                }
            }

            if (!renamingThis && ImGui::IsItemClicked()) {
                sceneManager.selectObject(obj->getId());
                objectSelected = true;
                showSelectedModelPanel = true;
            }

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Rename")) {
                    isRenaming = true;
                    renamingId = obj->getId();
                    strncpy(renameBuffer, obj->getName().c_str(), sizeof(renameBuffer) - 1);
                    renameBuffer[sizeof(renameBuffer) - 1] = '\0';
                }
                if (ImGui::MenuItem("Delete")) {
                    unsigned int idToRemove = obj->getId();
                    if (obj->getSelected()) {
                        objectSelected = false;
                        showSelectedModelPanel = false;
                    }
                    sceneManager.removeObject(idToRemove);
                    ImGui::EndPopup();
                    break;
                }
                ImGui::EndPopup();
            }
        }

        ImGui::Spacing();
        ImGui::TextDisabled("System (%zu):", systemObjects.size());

        for (const auto& obj : systemObjects) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (obj->getSelected()) flags |= ImGuiTreeNodeFlags_Selected;

            ImGui::TreeNodeEx(obj->getName().c_str(), flags);

            if (ImGui::IsItemClicked()) {
                sceneManager.selectObject(obj->getId());
                bool isLight = (obj->getId() == gDirLightObjectNumericId);
                objectSelected = !isLight;
                showSelectedModelPanel = !isLight;
            }
        }

        ImGui::EndChild();

        ImGui::Checkbox("Enable Picking", &pickingEnabled);
    }
}

// Rendering
void renderSectionRendering() {
    if (ImGui::CollapsingHeader("Rendering")) {
        static const char* renderModes[] = { "Solid", "Wireframe", "Solid + Wireframe" };
        ImGui::PushItemWidth(-1);
        if (ImGui::Combo("##RenderMode", &currentRenderMode, renderModes, IM_ARRAYSIZE(renderModes))) {
            Model::RenderMode mode = static_cast<Model::RenderMode>(currentRenderMode);
            if (modelManager.hasActiveModel()) {
                modelManager.setActiveModelRenderMode(mode);
            }
        }
        ImGui::PopItemWidth();
    }
}

// Camera
void renderSectionCamera() {
    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::Text("Distance:");
        ImGui::PushItemWidth(-1);
        if (ImGui::SliderFloat("##CamDist", &mouseControl.cameraDistance, 0.5f, 15.0f)) {
            updateCameraPosition();
        }
        ImGui::PopItemWidth();

        if (ImGui::Button("Reset View", ImVec2(-1, 28))) {
            resetCameraView();
        }
    }
}

// Lighting
void renderSectionLighting() {
    if (ImGui::CollapsingHeader("Lighting")) {
        ImGui::Text("Ambient:");
        ImGui::PushItemWidth(-1);
        ImGui::SliderFloat("##AmbIntensity", &gLighting.ambientIntensity, 0.0f, 1.0f);
        ImGui::ColorEdit3("##AmbColor", &gLighting.ambientColor.x, ImGuiColorEditFlags_NoInputs);
        ImGui::PopItemWidth();

        ImGui::Spacing();

        ImGui::Text("Directional:");
        ImGui::Checkbox("Enabled##DirLight", &gDirLight.enabled);
        ImGui::SameLine();
        ImGui::Checkbox("Shadows", &gDirLight.enablePlanarShadows);

        ImGui::PushItemWidth(-1);
        ImGui::ColorEdit3("##DirColor", &gDirLight.color.x, ImGuiColorEditFlags_NoInputs);
        ImGui::PopItemWidth();
    }
}

// Actions
void renderSectionActions() {
    ImGui::Text("Quick Actions:");

    if (ImGui::Button("Help", ImVec2(-1, 28))) {
        shoeHelpWindow = !shoeHelpWindow;
    }

    // Pulsante Console (prima era duplicato, ora è unico)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.60f, 0.80f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.70f, 0.90f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.50f, 0.70f, 1.0f));

    if (ImGui::Button("Console", ImVec2(-1, 28))) {
        gShowConsole = !gShowConsole;
    }

    ImGui::PopStyleColor(3);
}

int main() {
    std::cout << std::filesystem::current_path() << std::endl;

    Window win;
    if (!win.initialize(gFramebufferWidth, gFramebufferHeight, "3D Modeler")) return -1;

    // Carica l'icona della finestra
    GLFWimage icon;
    int iconWidth, iconHeight, iconChannels;
    unsigned char* iconData = stbi_load("Assets/Images/logo.png", &iconWidth, &iconHeight, &iconChannels, 4);
    if (iconData) {
        icon.width = iconWidth;
        icon.height = iconHeight;
        icon.pixels = iconData;
        glfwSetWindowIcon(win.getGLFWwindow(), 1, &icon);
        stbi_image_free(iconData);
    }

    Logger& logger = Logger::Get();
    logger.SetGlfwWindow(win.getGLFWwindow());
    logger.SetAppVersion("0.1.0");
    logger.SetNotionFormUrl("");
    logger.AddExtraSystemInfoLine("Build: Debug");
    logger.Log(Logger::Level::Info, "Applicazione avviata.");

    // Resto dell'inizializzazione invariato...
    glfwSetMouseButtonCallback(win.getGLFWwindow(), mouseButtonCallback);
    glfwSetCursorPosCallback(win.getGLFWwindow(), cursorPositionCallback);
    glfwSetScrollCallback(win.getGLFWwindow(), scrollCallback);
    glfwSetFramebufferSizeCallback(win.getGLFWwindow(), framebufferSizeCallback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(win.getGLFWwindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    ApplayModernRoundedStyle();

    // Customizza la title bar nativa (approccio A)
    #ifdef _WIN32
        HWND hwnd = glfwGetWin32Window(win.getGLFWwindow());
        if (hwnd) {
            // Abilita dark mode title bar
            BOOL useDarkMode = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

            // Personalizza i colori della title bar (Windows 11+)
            // Colore viola scuro coordinato con lo stile ImGui
            COLORREF titleBarColor = RGB(45, 45, 50);
            COLORREF titleTextColor = RGB(160, 140, 200);
            COLORREF borderColor = RGB(80, 80, 85);

            DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &titleBarColor, sizeof(titleBarColor));
            DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &titleTextColor, sizeof(titleTextColor));
            DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
        }
    #endif

    ConsoleWindow::Get().RegisterCommand("echo", "Ripete gli argomenti forniti.", [](const std::vector<std::string>& args) {
        std::string joined;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i) joined += ' ';
            joined += args[i];
        }
        ConsoleWindow::Get().PushConsoleLine("echo: " + joined);
    });

    // Carica gli shader
    //logger.CaptureGpuInfo();
    logger.Log(Logger::Level::Info, "Rilevate informazioni GPU.");

    GLuint shader = LoadShader("Shaders/Object.vert", "Shaders/Object.frag");
    if (shader == 0) {
        logger.Log(Logger::Level::Error, "Errore nel caricamento Object shader.");
        std::cerr << "Errore nel caricamento degli shader." << std::endl;
        return -1;
    }
    else {
        logger.Log(Logger::Level::Info, "Shader Object caricato.");
    }

    GLuint gridShader = LoadShader("Shaders/Grid.vert", "Shaders/Grid.frag");
    if (gridShader == 0) {
        logger.Log(Logger::Level::Error, "Errore nel caricamento Grid shader.");
        std::cerr << "Errore nel caricamento degli shader." << std::endl;
        return -1;
    }
    else {
        logger.Log(Logger::Level::Info, "Shader Grid caricato.");
    }

    GLuint dirShadowDepthShader = LoadShader("Shaders/DirShadowDepth.vert", "Shaders/DirShadowDepth.frag");
    if (dirShadowDepthShader == 0) {
        logger.Log(Logger::Level::Warn, "Shader depth luce non caricato.");
        std::cerr << "Errore shader depth luce\n";
    }
    else {
        logger.Log(Logger::Level::Info, "Shader depth luce caricato.");
    }

    // Registra i modelli base
    modelManager.registerModel(std::make_shared<CubeModel>());
    modelManager.registerModel(std::make_shared<SphereModel>());
    modelManager.registerModel(std::make_shared<PyramidModel>());
    modelManager.registerModel(std::make_shared<ImagePlaneModel>());

    // Inizializza i modelli registrati
    modelManager.initializeModels();
    logger.Log(Logger::Level::Info, "Modelli base registrati.");

    // Inizializza il sistema di picking
    initializePickingSystem();
    logger.Log(Logger::Level::Info, "Picking inizializzato.");

    // Imposta il modello della griglia
    Grid grid;
    grid.initialize();
    std::string selectedModel = "";
    logger.Log(Logger::Level::Info, "Griglia inizializzata.");

    // Gizmo luce direzionale
    glm::vec3 lightInitialPos = gDirLight.position;
    auto lightObj = sceneManager.addSystemObject("Sphere", lightInitialPos, gDirLightObjectNumericId, "Directional Light");
    if (lightObj) {
        sceneManager.updateObjectScale(gDirLightObjectNumericId, glm::vec3(0.15f));
        logger.Log(Logger::Level::Info, "Oggetto luce direzionale aggiunto.");
    }

    {
        // Posizione iniziale del bersaglio: usa gDirLight.target ma forza y a 0
        gDirLight.target = ClampTargetOnFloor(gDirLight.target);
        auto targetObj = sceneManager.addSystemObject("ImagePlane", gDirLight.target, gDirLightTargetObjectNumericId, "Light Target");
        if (targetObj) {
            sceneManager.updateObjectScale(gDirLightTargetObjectNumericId, glm::vec3(0.15f, 0.15f, 0.15f));
            glm::quat rot = glm::quat(glm::radians(glm::vec3(-90.0f, 0.0f, 0.0f)));
            sceneManager.updateObjectRotation(gDirLightTargetObjectNumericId, rot);
            if (auto t = findObjectById(gDirLightTargetObjectNumericId)) {
                t->setOverrideColor(glm::vec4(1.0f, 140.0f / 255.0f, 0.0f, 1.0f));
            }
            logger.Log(Logger::Level::Info, "Target luce direzionale aggiunto.");
        }
    }

    // Render loop
    while (!win.shouldClose()) {
        // Timer
        static float lastFrame = 0.0f;
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        gTime += deltaTime;
        frameTimeAccumulator += deltaTime;
        if (++frameCount && frameTimeAccumulator >= 0.5f) {
            fps = frameCount * 2.0f;
            frameTimeAccumulator = 0.0f;
            frameCount = 0;
        }

        // Abilita il test di profondità
        glEnable(GL_DEPTH_TEST);

        // Clear the screen
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Input & eventi
        win.processInput();
        win.pollEvents();

        // Framebuffer size / aspect
        int fbw, fbh;
        glfwGetFramebufferSize(win.getGLFWwindow(), &fbw, &fbh);
        if (fbw == 0 || fbh == 0) { glfwWaitEvents(); continue; }
        float aspect = static_cast<float>(fbw) / static_cast<float>(fbh);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(
            mouseControl.camPos,
            mouseControl.cameraTarget,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

		updateFiniteLimitState();

        // Avvio frame ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        float vw = ImGui::GetIO().DisplaySize.x;
        float vh = ImGui::GetIO().DisplaySize.y;
        const float SIDEBAR_LEFT_WIDTH = 260.0f;
        const float SIDEBAR_MARGIN = 6.0f;

        // Left sidebar
        ImGui::SetNextWindowPos(ImVec2(SIDEBAR_MARGIN, SIDEBAR_MARGIN), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(SIDEBAR_LEFT_WIDTH, vh - (SIDEBAR_MARGIN * 2)), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));

        ImGuiWindowFlags sidebarFlags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("##LeftSidebar", nullptr, sidebarFlags)) {

            // Grid Mode
            renderSectionGridMode();

            ImGui::Spacing();
            renderSeparator();
            ImGui::Spacing();

            // Scene Objects
            renderSectionSceneObjects();

            ImGui::Spacing();
            renderSeparator();
            ImGui::Spacing();

            // Rendering
            renderSectionRendering();

            ImGui::Spacing();
            renderSeparator();
            ImGui::Spacing();

            // Camera
            renderSectionCamera();

            ImGui::Spacing();
            renderSeparator();
            ImGui::Spacing();

            // Lighting
            renderSectionLighting();

            ImGui::Spacing();
            renderSeparator();
            ImGui::Spacing();

            // Actions
            renderSectionActions();

            ImGui::End();
        }

        ImGui::PopStyleVar(2);

        // Pannello per il modello selezionato
        auto selObjForPanel = sceneManager.getSelectedObject();
        bool lightSelected = selObjForPanel && (selObjForPanel->getId() == gDirLightObjectNumericId);
        bool lightTargetSelected = selObjForPanel && (selObjForPanel->getId() == gDirLightTargetObjectNumericId);

        if (showSelectedModelPanel && !lightSelected && !lightTargetSelected) {
            float detailsWidth = std::clamp(vw * 0.25f, 300.0f, 500.0f);
            ImVec2 minSize = ImVec2(300, 550);
            ImGui::SetNextWindowSizeConstraints(minSize, ImVec2(FLT_MAX, FLT_MAX));

            if (!pinSelectedModelPanel) {
                ImVec2 anchorPos(ImGui::GetMainViewport()->Pos.x + vw - 12.0f,
                    ImGui::GetMainViewport()->Pos.y + 60.0f);
                ImGui::SetNextWindowPos(anchorPos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
                ImGui::SetNextWindowSize(ImVec2(detailsWidth, 0.0f), ImGuiCond_Always);
            }
            else {
                ImGui::SetNextWindowSize(ImVec2(detailsWidth, 0.0f), ImGuiCond_FirstUseEver);
            }

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

                    auto selectedObj = sceneManager.getSelectedObject();
                    bool isLight = (selectedObj->getId() == gDirLightObjectNumericId);

                    ImGui::Text("Selected: %s", selectedObj->getName().c_str());
                    ImGui::Separator();

                    // Posizione
                    ImGui::Text("Position:");
                    glm::vec3 position = selectedObj->getPosition();

                    bool posChanged = false;

                    // Layout input X, Y, Z
                    float columnWidth = ImGui::GetContentRegionAvail().x / 3;

                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("X:"); ImGui::SameLine();
                    posChanged |= ImGui::InputFloat("##posX", &position.x, 0.0f, 0.0f, "%.2f");
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("Y:"); ImGui::SameLine();
                    posChanged |= ImGui::InputFloat("##posY", &position.y, 0.0f, 0.0f, "%.2f");
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    ImGui::PushItemWidth(columnWidth - 10);
                    ImGui::Text("Z:"); ImGui::SameLine();
                    posChanged |= ImGui::InputFloat("##posZ", &position.z, 0.0f, 0.0f, "%.2f");
                    ImGui::PopItemWidth();

                    if (posChanged) {
                        sceneManager.updateObjectPosition(selectedObj->getId(), position);
                        if (isLight) gDirLight.position = position;
                    }
                    if (ImGui::Button("Reset Position", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                        sceneManager.resetObjectToInitialPosition(selectedObj->getId());
                    }

					// Controllo dei confini se la griglia è finita
                    if (posChanged) {
                        glm::vec3 clamped = !gInfiniteGrid ? clampToFiniteSpace(position) : position;
                        if (clamped != position) {
                            gLastMovementClamped = true;
                            gBoundaryWarningMessage = "Movimento limitato: il modello non puo' uscire dai confini.";
                            position = clamped;
                        }
                        else {
                            gLastMovementClamped = false;
                        }
                        sceneManager.updateObjectPosition(selectedObj->getId(), position);
                    }

                    glm::vec3 clampedPos = !gInfiniteGrid ? clampToFiniteSpace(position) : position;
                    if (clampedPos != position) {
                        gLastMovementClamped = true;
                        gBoundaryWarningMessage = "Transform clamped ai limiti dell'area.";
                        position = clampedPos;
                    }
                    else {
                        gLastMovementClamped = false;
                    }
                    sceneManager.updateObjectPosition(selectedObj->getId(), position);

                    if (!isLight) {
                        // Scala
                        ImGui::Separator();
                        ImGui::Text("Scale:");

                        glm::vec3 scale = selectedObj->getScale();
                        bool scaleChanged = false;

                        ImGui::PushItemWidth(columnWidth - 10);
                        ImGui::Text("X:"); ImGui::SameLine();
                        scaleChanged |= ImGui::InputFloat("##scaleX", &scale.x, 0.0f, 0.0f, "%.2f");
                        ImGui::PopItemWidth();
                        ImGui::SameLine();
                        ImGui::PushItemWidth(columnWidth - 10);
                        ImGui::Text("Y:"); ImGui::SameLine();
                        scaleChanged |= ImGui::InputFloat("##scaleY", &scale.y, 0.0f, 0.0f, "%.2f");
                        ImGui::PopItemWidth();
                        ImGui::SameLine();
                        ImGui::PushItemWidth(columnWidth - 10);
                        ImGui::Text("Z:"); ImGui::SameLine();
                        scaleChanged |= ImGui::InputFloat("##scaleZ", &scale.z, 0.0f, 0.0f, "%.2f");
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

                        displayedEulerAngles = glm::degrees(glm::eulerAngles(selectedObj->getRotation()));

                        bool rotChanged = false;

                        ImGui::PushItemWidth(columnWidth - 10);
                        ImGui::Text("X:"); ImGui::SameLine();
                        rotChanged |= ImGui::InputFloat("##rotX", &displayedEulerAngles.x, 0.0f, 0.0f, "%.2f");
                        ImGui::PopItemWidth();

                        ImGui::SameLine();
                        ImGui::PushItemWidth(columnWidth - 10);
                        ImGui::Text("Y:"); ImGui::SameLine();
                        rotChanged |= ImGui::InputFloat("##rotY", &displayedEulerAngles.y, 0.0f, 0.0f, "%.2f");
                        ImGui::PopItemWidth();

                        ImGui::SameLine();
                        ImGui::PushItemWidth(columnWidth - 10);
                        ImGui::Text("Z:"); ImGui::SameLine();
                        rotChanged |= ImGui::InputFloat("##rotZ", &displayedEulerAngles.z, 0.0f, 0.0f, "%.2f");
                        ImGui::PopItemWidth();

                        if (rotChanged) {
                            glm::quat rotation = glm::quat(glm::radians(displayedEulerAngles));
                            glm::vec3 pos = selectedObj->getPosition();
                            if (pos.y < 0.0f) pos.y = 0.0f;
                            pos = clampToFiniteSpace(pos);

                            sceneManager.updateObjectPosition(selectedObj->getId(), pos);
                            sceneManager.updateObjectRotation(selectedObj->getId(), rotation);
                        }

                        if (ImGui::Button("Reset Rotation", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                            sceneManager.updateObjectRotation(selectedObj->getId(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                            displayedEulerAngles = glm::vec3(0.0f);
                        }
                    }

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
                    }
                    ImGui::Spacing();

                    ImGui::Separator();
                    ImGui::Text("Appearance:");

                    if (!isLight)
                    {
                        if (ImGui::Button("Load Texture", ImVec2(ImGui::GetContentRegionAvail().x, 35))) {
                            std::string texturePath;
                            GLuint textureID;
                            bool needsConfirmation = false;

                            if (ModelLoader::openTextureFile(modelManager, sceneManager,
                                texturePath, textureID, needsConfirmation)) {
                                if (needsConfirmation && !dontAskTextureReplace) {
                                    textureReplaceDialog.show = true;
                                    textureReplaceDialog.texturePath = texturePath;
                                    textureReplaceDialog.newTextureID = textureID;
                                }
                                else if (needsConfirmation && dontAskTextureReplace) {
                                    selectedObj->setOverrideTexture(textureID);
                                    selectedObj->clearOverrideColor();
                                }
                            }
                            else {
                                if (!texturePath.empty()) {
                                    errorDialog.show = true;
                                    errorDialog.title = "Texture Loading Error";
                                    errorDialog.message = "Unable to load the texture from the selected file.";
                                }
                            }
                        }

                        // Pulsante per aprire color picker
                        static bool showColorPicker = false;
                        if (ImGui::Button("Choose Color", ImVec2(ImGui::GetContentRegionAvail().x, 35))) {
                            showColorPicker = !showColorPicker;
                        }
                        if (showColorPicker) {
                            ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - 10,
                                                           ImGui::GetWindowPos().y + 200), ImGuiCond_FirstUseEver);
                            ImGui::SetNextWindowSize(ImVec2(350, 450), ImGuiCond_FirstUseEver);
                            if (ImGui::Begin("Color Picker", &showColorPicker, ImGuiWindowFlags_NoCollapse)) {
                                static glm::vec4 color = selectedObj->hasOverrideColor()
                                                         ? selectedObj->getOverrideColor()
                                                         : glm::vec4(1.0f);
                                ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaBar |
                                                            ImGuiColorEditFlags_AlphaPreview |
                                                            ImGuiColorEditFlags_DisplayRGB |
                                                            ImGuiColorEditFlags_DisplayHSV |
                                                            ImGuiColorEditFlags_PickerHueWheel;


                                if (ImGui::ColorPicker4("##ColorPicker", &color.x, flags)) {
                                    selectedObj->setOverrideColor(color);
                                }


                                ImGui::Separator();
                                ImGui::Text("Preset Colors:");

                                float buttonSize = 40.0f;
                                float spacing = 5.0f;
                                auto preset = [&](const char* id, const ImVec4& c){
                                    if (ImGui::ColorButton(id, c, 0, ImVec2(buttonSize, buttonSize))) {
                                        color = glm::vec4(c.x, c.y, c.z, c.w);
                                        selectedObj->setOverrideColor(color);
                                    }
                                };
                                preset("##Red", ImVec4(1,0,0,1)); ImGui::SameLine(0,spacing);
                                preset("##Green", ImVec4(0,1,0,1)); ImGui::SameLine(0,spacing);
                                preset("##Blue", ImVec4(0,0,1,1)); ImGui::SameLine(0,spacing);
                                preset("##Yellow", ImVec4(1,1,0,1));
                                preset("##Cyan", ImVec4(0,1,1,1)); ImGui::SameLine(0,spacing);
                                preset("##Magenta", ImVec4(1,0,1,1)); ImGui::SameLine(0,spacing);
                                preset("##White", ImVec4(1,1,1,1)); ImGui::SameLine(0,spacing);
                                preset("##Black", ImVec4(0,0,0,1));
                                preset("##Orange", ImVec4(1,0.5f,0,1)); ImGui::SameLine(0,spacing);
                                preset("##Purple", ImVec4(0.5f,0,1,1)); ImGui::SameLine(0,spacing);
                                preset("##Pink", ImVec4(1,0,0.5f,1)); ImGui::SameLine(0,spacing);
                                preset("##Gray", ImVec4(0.5f,0.5f,0.5f,1));

                                ImGui::Separator();
                                if (ImGui::Button("Reset to Default", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                                    selectedObj->clearOverrideColor();
                                    selectedObj->clearOverrideTexture();
                                    color = glm::vec4(1.0f);
                                }
                                ImGui::End();
                            }
                        }

                        ImGui::Spacing();

                        // Stato Appearance
                        auto modelPtr = selectedObj->getModel();
                        if (selectedObj->hasOverrideTexture()) {
                            ImGui::TextColored(ImVec4(0.0f,1.0f,0.0f,1.0f), "Custom Texture Active");
                            ImGui::SameLine();
                            if (ImGui::SmallButton("Remove##Texture")) {
                                selectedObj->clearOverrideTexture();
                            }
                        } else if (selectedObj->hasOverrideColor()) {
                            ImGui::TextColored(ImVec4(0.0f,1.0f,0.0f,1.0f), "Custom Color Active");
                            ImGui::SameLine();
                            if (ImGui::SmallButton("Remove##Color")) {
                                selectedObj->clearOverrideColor();
                            }
                        } else if (modelPtr->hasTexture()) {
                            ImGui::TextColored(ImVec4(0.7f,0.7f,1.0f,1.0f), "Model Default Texture");
                        } else {
                            ImGui::TextColored(ImVec4(0.7f,0.7f,0.7f,1.0f), "Default Appearance");
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("Directional Light: non supporta texture o colori.");
                    }

                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

                    if (isLight) {
                        ImGui::BeginDisabled();
                        ImGui::Button("Delete Object", ImVec2(ImGui::GetContentRegionAvail().x, 35));
                        ImGui::EndDisabled();
                    } else {
                        if (ImGui::Button("Delete Object", ImVec2(ImGui::GetContentRegionAvail().x, 35))) {
                            showDeleteConfirmation = true;
                            objectToDeleteId = selectedObj->getId();
                        }
                    }
                    ImGui::PopStyleColor(3);
                }
                else {
                    ImGui::Text("No object selected");
                    if (!pinSelectedModelPanel) {
                        showSelectedModelPanel = false;
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
                ImGui::BulletText("Orbital motion: ALT + left click drag");
                ImGui::BulletText("Pan view: CTRL + left click drag");
                ImGui::BulletText("Zoom: scroll wheel");
                ImGui::BulletText("Reset view: 'Reset View'");
                ImGui::Separator();
                ImGui::Text("Model manipulation:");
                ImGui::BulletText("Select: left click");
                ImGui::BulletText("Move: SHIFT + drag");
                ImGui::BulletText("Move X: SHIFT + Q + drag");
                ImGui::BulletText("Move Y: SHIFT + W + drag");
                ImGui::BulletText("Move Z: SHIFT + E + drag");
                ImGui::BulletText("Rotate: ImGuizmo rotate mode");
                ImGui::Separator();
                ImGui::Text("Scene management:");
                ImGui::BulletText("Add object: panel 'Add Object'");
                ImGui::BulletText("Delete object: context menu");
                ImGui::BulletText("Duplicate object: CTRL+D");
                ImGui::BulletText("Select all: CTRL+A");
                ImGui::BulletText("Delete selected: DELETE");
                ImGui::Separator();
                if (ImGui::Button("Close", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) shoeHelpWindow = false;
                ImGui::End();
            }
        }

        // FPS display
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 120, 8), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(110, 0), ImGuiCond_Always);
        ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("FPS: %.1f", fps);
        ImGui::End();

        if (showFiniteSpaceInfoWindow && !gInfiniteGrid) {
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 340, 60), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(330, 0), ImGuiCond_Always);
            if (ImGui::Begin("Limiti Spazio", &showFiniteSpaceInfoWindow,
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings)) {
                float half = gGridHalfSize + gGridBoundaryMargin;
                ImGui::TextWrapped("Modalita' Finita attiva.\nArea XZ: [-%.2f, %.2f].\nLimite oggetti raggiunto",
                                   half, half, gFiniteModeMaxObjects);
                if (gLastPlacementClamped || gLastMovementClamped) {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.95f,0.4f,0.2f,1.0f), "%s", gBoundaryWarningMessage.c_str());
                }
                ImGui::Separator();
                if (ImGui::Button("Chiudi", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    showFiniteSpaceInfoWindow = false;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    showFiniteSpaceInfoWindow = false;
                }
                ImGui::End();
            }
        }

        // Finestra modale di conferma eliminazione
        if (showDeleteConfirmation) {
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

            ImGui::OpenPopup("Delete Object?");

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

            // Usa BeginPopupModal per creare la finestra di conferma
            if (ImGui::BeginPopupModal("Delete Object?", NULL,
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings)) {
                ImGui::Text("Are you sure you want to delete this object?");
                ImGui::Text("This operation cannot be undone.");
                ImGui::Separator();

                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

                float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2) / 3;

                if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
                    showDeleteConfirmation = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

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

        // Console Window
        if (gShowConsole) {
            ImGui::SetNextWindowSize(ImVec2(900, 550), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
            ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.25f, 0.25f, 0.27f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.35f, 0.35f, 0.40f, 1.0f));

            if (ImGui::Begin("Developer Console", &gShowConsole, ImGuiWindowFlags_NoCollapse)) {

                if (ImGui::BeginTabBar("##DevTools", ImGuiTabBarFlags_None)) {

                    // Tab Console Commands
                    if (ImGui::BeginTabItem("Console")) {
                        ConsoleWindow::Get().Draw();
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::End();
            }

            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();
        }

        // Finestra modale di errore
        if (errorDialog.show) {
            // Centra la finestra di dialogo
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

            // Imposta dimensione minima e massima per il dialogo
            ImGui::SetNextWindowSizeConstraints(ImVec2(500, 150), ImVec2(800, 400));

            // Assicurati di aprire il popup PRIMA di BeginPopupModal
            ImGui::OpenPopup(errorDialog.title.c_str());

            // Imposta lo stile della finestra modale
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(25, 20));
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));

            // Usa BeginPopupModal per creare la finestra di errore
            if (ImGui::BeginPopupModal(errorDialog.title.c_str(), NULL,
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings)) {

                // Layout migliorato con wrap del testo
                ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x - 10);

                // Icona di errore (simbolo)
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "ERROR");
                ImGui::SameLine();
                ImGui::TextWrapped("%s", errorDialog.message.c_str());

                if (!errorDialog.details.empty()) {
                    ImGui::Separator();
                    ImGui::TextWrapped("%s", errorDialog.details.c_str());
                }

                ImGui::PopTextWrapPos();

                ImGui::Separator();
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

                // Pulsante OK centrato
                float buttonWidth = 120.0f;
                float windowWidth = ImGui::GetContentRegionAvail().x;
                ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

                if (ImGui::Button("OK", ImVec2(buttonWidth, 30)) ||
                    ImGui::IsKeyPressed(ImGuiKey_Enter) ||
                    ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    errorDialog.show = false;
                    errorDialog.title.clear();
                    errorDialog.message.clear();
                    errorDialog.details.clear();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();
        }

        // Finestra di conferma sostituzione texture
        if (textureReplaceDialog.show) {
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

            // Imposta dimensione adeguata per il contenuto
            ImGui::SetNextWindowSizeConstraints(ImVec2(600, 200), ImVec2(900, 400));

            ImGui::OpenPopup("Replace Texture?");

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(25, 20));
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.6f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.3f, 0.7f, 0.5f, 1.0f));

            if (ImGui::BeginPopupModal("Replace Texture?", NULL,
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings)) {

                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "WARNING");
                ImGui::SameLine();
                ImGui::TextWrapped("This object already has a texture applied.");

                ImGui::Spacing();
                ImGui::TextWrapped("Do you want to replace it with the new texture?");

                ImGui::Separator();
                ImGui::Text("New texture:");
                ImGui::Indent(20);
                ImGui::BulletText("%s", std::filesystem::path(textureReplaceDialog.texturePath).filename().string().c_str());
                ImGui::Unindent(20);

                ImGui::Separator();
                ImGui::Text("Current texture:");
                ImGui::Indent(20);
                ImGui::BulletText("%s", "Default or Custom Texture");
                ImGui::Unindent(20);

                ImGui::Separator();
                ImGui::Text("Texture Usage:");
                ImGui::Indent(20);
                ImGui::BulletText("The new texture will replace the existing one.");
                ImGui::Unindent(20);

                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

                // Layout pulsanti migliorato
                float totalWidth = ImGui::GetContentRegionAvail().x;
                float buttonWidth = (totalWidth - 10) / 3; // 3 pulsanti con spacing

                // Pulsante "Don't Replace"
                if (ImGui::Button("Don't Replace", ImVec2(buttonWidth, 35))) {
                    if (textureReplaceDialog.newTextureID != 0) {
                        glDeleteTextures(1, &textureReplaceDialog.newTextureID);
                    }

                    textureReplaceDialog.show = false;
                    textureReplaceDialog.texturePath = "";
                    textureReplaceDialog.newTextureID = 0;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                // Pulsante "Don't Ask Again"
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.4f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.5f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.3f, 0.0f, 1.0f));

                if (ImGui::Button("Don't Ask Again", ImVec2(buttonWidth, 35))) {
                    dontAskTextureReplace = true;
                    ModelLoader::applyTextureToSelected(sceneManager, textureReplaceDialog.newTextureID);

                    textureReplaceDialog.show = false;
                    textureReplaceDialog.texturePath = "";
                    textureReplaceDialog.newTextureID = 0;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::PopStyleColor(3);

                ImGui::SameLine();

                // Pulsante "Yes, Replace"
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.2f, 1.0f));

                if (ImGui::Button("Yes, Replace", ImVec2(buttonWidth, 35))) {
                    ModelLoader::applyTextureToSelected(sceneManager, textureReplaceDialog.newTextureID);

                    textureReplaceDialog.show = false;
                    textureReplaceDialog.texturePath = "";
                    textureReplaceDialog.newTextureID = 0;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::PopStyleColor(3);

                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    if (textureReplaceDialog.newTextureID != 0) {
                        glDeleteTextures(1, &textureReplaceDialog.newTextureID);
                    }

                    textureReplaceDialog.show = false;
                    textureReplaceDialog.texturePath = "";
                    textureReplaceDialog.newTextureID = 0;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();
        }

        {
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoCollapse;
            const float pad = 16.0f;
            const ImVec2 btnSize(56, 56);

            ImVec2 vpPos = ImGui::GetMainViewport()->Pos;
            ImVec2 vpSize = ImGui::GetMainViewport()->Size;
            ImVec2 winPos = ImVec2(vpPos.x + vpSize.x - btnSize.x - pad,  // X: angolo in basso a destra
                vpPos.y + vpSize.y - btnSize.y - pad); // Y

            ImGui::SetNextWindowPos(winPos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(btnSize, ImGuiCond_Always);
            ImGui::Begin("##BugReportFloating", nullptr, flags);

            // Area cliccabile
            ImGui::InvisibleButton("##bug_btn", btnSize);
            bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            bool clicked = ImGui::IsItemClicked();

            // Disegno cerchio e icona
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            ImVec2 center = ImVec2(0.5f * (min.x + max.x), 0.5f * (min.y + max.y));
            float radius = 0.5f * btnSize.x;

            ImU32 bgCol = hovered ? IM_COL32(230, 90, 60, 255) : IM_COL32(255, 120, 80, 255);
            ImU32 bdCol = IM_COL32(35, 35, 35, 255);
            dl->AddCircleFilled(center, radius, bgCol, 64);
            dl->AddCircle(center, radius, bdCol, 64, 2.0f);

            // Semplice icona “!” centrata
            const char* icon = "!";
            ImVec2 tSize = ImGui::CalcTextSize(icon);
            ImVec2 tPos(center.x - tSize.x * 0.5f, center.y - tSize.y * 0.65f);
            dl->AddText(tPos, IM_COL32(255, 255, 255, 255), icon);

            if (hovered) {
                ImGui::SetTooltip("Apri form Bug Report");
            }
            if (clicked) {
                // Genera il report e apre il form online (usa l’URL impostato con SetNotionFormUrl)
                Logger::Get().ExportAndOpenReport(true);
            }

            ImGui::End();
        }

        // Aggiorna Shadow System (matrici luce dinamiche)
        glm::vec3 lightDir;
        if (gDirLight.useTarget) {
            lightDir = glm::normalize(gDirLight.target - gDirLight.position);
        }
        else {
            glm::vec3 e = glm::radians(gDirLight.orientationEuler);
            glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), e.y, glm::vec3(0, 1, 0));
            glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), e.x, glm::vec3(1, 0, 0));
            lightDir = glm::normalize(glm::vec3(rotY * rotX * glm::vec4(0, 0, -1, 0)));
        }

        gShadows.updateMatrices(sceneManager, gDirLightObjectNumericId);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, fbw, fbh);
        glEnable(GL_DEPTH_TEST);

        // Griglia
        grid.render(gridShader, projection, view, mouseControl.camPos,
            gInfiniteGrid, gGridHalfSize,
            GRID_COLOR, GRID_X_AXIS_COLOR, GRID_Z_AXIS_COLOR);

        // Scene
        renderScene(shader, view, projection);

        drawLightDirectionOverlay(view, projection);
        if (objectSelected) {
            renderImGuizmo(view, projection);
        }

        renderLoadingDialog();
        renderExportFormatDialog();
        renderFiniteLimitDialog();
        renderGridModeSwitchErrorDialog();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        win.swapBuffers();
    }

    // Pulisci le risorse
    modelManager.cleanup();
    pickingBuffer.cleanup();
    grid.cleanup();
    gShadows.shutdown();

    // Chiudi ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteProgram(shader);
    glDeleteProgram(gridShader);
    glDeleteProgram(pickingShader);
    glDeleteProgram(dirShadowDepthShader);

    glfwTerminate();
    return 0;
}

// Renderizza ImGuizmo per il modello selezionato
void renderImGuizmo(const glm::mat4& view, const glm::mat4& projection) {
    auto selectedObj = sceneManager.getSelectedObject();
    if (!selectedObj || !objectSelected) return;

    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImGuizmo::SetRect(0, 0, (float)gFramebufferWidth, (float)gFramebufferHeight);

    glm::mat4 modelMatrix = selectedObj->getModelMatrix();
    const bool isLight = (selectedObj->getId() == gDirLightObjectNumericId);
    const bool isLightTarget = (selectedObj->getId() == gDirLightTargetObjectNumericId);
    const bool isSystem = (isLight || isLightTarget);

    ImGuizmo::OPERATION op = isSystem ? ImGuizmo::TRANSLATE : currentGizmoOperation;

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        op,
        currentGizmoMode,
        glm::value_ptr(modelMatrix),
        nullptr,
        useSnap ? snapValues : nullptr
    );

    if (ImGuizmo::IsUsing()) {
        // Decomponi la matrice trasformata
        glm::vec3 position, eulerDeg, scale;
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(modelMatrix),
            glm::value_ptr(position),
            glm::value_ptr(eulerDeg),
            glm::value_ptr(scale)
        );

        if (isLight) {
            // Vincoli luce
            if (position.y < 0.0f) position.y = 0.0f;
            position = clampToFiniteSpace(position);
            sceneManager.updateObjectPosition(selectedObj->getId(), position);
            gDirLight.position = position;
        }
        else if (isLightTarget) {
            // Target sempre sul piano XZ, rotazione forzata flat
            position = ClampTargetOnFloor(position);
            sceneManager.updateObjectPosition(selectedObj->getId(), position);

            glm::quat flatRot = glm::quat(glm::radians(glm::vec3(-90.0f, 0.0f, 0.0f)));
            sceneManager.updateObjectRotation(selectedObj->getId(), flatRot);

            gDirLight.target = position;
        }
        else {
            if (position.y < 0.0f) position.y = 0.0f;
            position = clampToFiniteSpace(position);
            sceneManager.updateObjectPosition(selectedObj->getId(), position);

            displayedEulerAngles = eulerDeg; // sincronizza UI con gizmo
            glm::quat rotation = glm::quat(glm::radians(displayedEulerAngles));
            sceneManager.updateObjectRotation(selectedObj->getId(), rotation);

            sceneManager.updateObjectScale(selectedObj->getId(), scale);
        }
    }

    // Mantieni vincoli post-manipolazione
    if (auto lightObj = findObjectById(gDirLightObjectNumericId)) {
        glm::vec3 lp = clampToFiniteSpace(lightObj->getPosition());
        if (lp.y < 0.0f) lp.y = 0.0f;
        if (lp != lightObj->getPosition()) {
            sceneManager.updateObjectPosition(gDirLightObjectNumericId, lp);
        }
        gDirLight.position = lp;
    }
    if (auto targetObj = findObjectById(gDirLightTargetObjectNumericId)) {
        glm::vec3 tp = ClampTargetOnFloor(targetObj->getPosition());
        if (tp != targetObj->getPosition()) {
            sceneManager.updateObjectPosition(gDirLightTargetObjectNumericId, tp);
        }
        gDirLight.target = tp;
    }
}

// Renderizza il dialog di caricamento
void renderLoadingDialog() {
    if (!loadingDialog.show) return;
    
    // Aggiorna animazione spinner
    loadingDialog.spinnerAngle += ImGui::GetIO().DeltaTime * 360.0f;
    if (loadingDialog.spinnerAngle > 360.0f) {
        loadingDialog.spinnerAngle -= 360.0f;
    }
    
    // Centra il dialog
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 0), ImGuiCond_Always);
    
    // Styling
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    
    // Colore titolo in base allo stato
    if (loadingDialog.hasError) {
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
    } else if (loadingDialog.isComplete) {
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
    }
    
    // Dialog non chiudibile durante il caricamento
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoCollapse;
    
    if (!loadingDialog.isComplete && !loadingDialog.hasError) {
        flags |= ImGuiWindowFlags_NoSavedSettings;
    }
    
    bool dialogOpen = true;
    if (ImGui::Begin("Loading Model", &dialogOpen, flags)) {
        
        // Nome file
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "File:");
        ImGui::SameLine();
        ImGui::TextWrapped("%s", std::filesystem::path(loadingDialog.fileName).filename().string().c_str());
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (loadingDialog.hasError) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "ERROR");
            ImGui::Spacing();
            ImGui::TextWrapped("%s", loadingDialog.errorMessage.c_str());
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            float buttonWidth = 120.0f;
            float windowWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

            if (ImGui::Button("Close", ImVec2(buttonWidth, 30))) {
                loadingDialog.reset();
            }
            
        } else if (loadingDialog.isComplete) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loading Complete!");
            
            ImGui::Spacing();
            ImGui::Text("Loaded in %.2f seconds", loadingDialog.elapsedTime);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            float buttonWidth = 120.0f;
            float windowWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
            
            if (ImGui::Button("OK", ImVec2(buttonWidth, 30))) {
                loadingDialog.reset();
            }
            
        } else {
            // Spinner animato
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImVec2 spinnerCenter = ImVec2(
                cursorPos.x + ImGui::GetContentRegionAvail().x * 0.5f,
                cursorPos.y + 40
            );
            
            float radius = 20.0f;
            int segments = 12;
            float thickness = 3.0f;
            
            // Disegna cerchio background
            draw_list->AddCircle(spinnerCenter, radius, 
                                ImColor(0.4f, 0.4f, 0.4f, 0.3f), segments, thickness);
            
            // Disegna arco animato
            float angle_rad = glm::radians(loadingDialog.spinnerAngle);
            constexpr float arc_length = glm::radians(270.0f); // 3/4 di cerchio
            
            ImVec2 arc_start = ImVec2(
                spinnerCenter.x + radius * cosf(angle_rad),
                spinnerCenter.y + radius * sinf(angle_rad)
            );
            
            draw_list->PathArcTo(spinnerCenter, radius, angle_rad, 
                               angle_rad + arc_length, segments);
            draw_list->PathStroke(ImColor(0.2f, 0.6f, 1.0f, 1.0f), 0, thickness);
            
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 80);
            
            // Status message
            ImGui::TextWrapped("%s", loadingDialog.statusMessage.c_str());
            
            ImGui::Spacing();
            
            // Progress bar
            char progressLabel[32];
            snprintf(progressLabel, sizeof(progressLabel), "%.0f%%", loadingDialog.progress * 100.0f);
            
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
            ImGui::ProgressBar(loadingDialog.progress, ImVec2(-1, 30), progressLabel);
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            
            // Elapsed time
            ImGui::Text("Elapsed: %.1f s", loadingDialog.elapsedTime);
            
            // Pulsante Cancel (opzionale)
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            float buttonWidth = 100.0f;
            float windowWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
            
            if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
                loadingDialog.error("Loading cancelled by user");
            }
        }
        ImGui::End();
    }
    if (!dialogOpen && (loadingDialog.isComplete || loadingDialog.hasError)) {
        loadingDialog.reset();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void renderSuccessDialog() {
    if (!successDialog.show) return;

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(500, 150), ImVec2(800, 400));

    ImGui::OpenPopup(successDialog.title.c_str());

    // Stile VERDE per successo
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(25, 20));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));

    if (ImGui::BeginPopupModal(successDialog.title.c_str(), &successDialog.show,
        ImGuiWindowFlags_NoResize)) {

        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x - 10);
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "SUCCESS");
        ImGui::SameLine();
        ImGui::TextWrapped("%s", successDialog.message.c_str());

        if (!successDialog.details.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("%s", successDialog.details.c_str());
        }
        ImGui::PopTextWrapPos();
        ImGui::Separator();
        float buttonWidth = 120.0f;
        float windowWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("OK", ImVec2(buttonWidth,30)) ||
            ImGui::IsKeyPressed(ImGuiKey_Enter) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            successDialog.reset();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

// Mostra informazioni dettagliate sull'oggetto selezionato
void showObjectDetailsPanel(unsigned int objectId) {
    auto obj = findObjectById(objectId);
    if (!obj) return;

    ImGui::Begin("Object Details", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Object ID: %u", obj->getId());
    ImGui::Text("Object Name: %s", obj->getName().c_str());
    ImGui::Separator();

    // Mostra posizione, rotazione, scala con set di controlli
    glm::vec3 position = obj->getPosition();
    glm::quat rotation = obj->getRotation();
    glm::vec3 scale = obj->getScale();

    ImGui::Text("Position:");
    ImGui::InputFloat3("##position", glm::value_ptr(position));
    ImGui::Text("Rotation (degrees):");
    ImGui::InputFloat3("##rotation", glm::value_ptr(displayedEulerAngles));
    ImGui::Text("Scale:");
    ImGui::InputFloat3("##scale", glm::value_ptr(scale));

    if (ImGui::Button("Apply Changes")) {
        // Applica le modifiche solo se ci sono cambiamenti reali
        bool changed = false;
        changed |= (position != obj->getPosition());
        changed |= (rotation != obj->getRotation());
        changed |= (scale != obj->getScale());

        if (changed) {
            sceneManager.updateObjectPosition(obj->getId(), position);
            sceneManager.updateObjectRotation(obj->getId(), rotation);
            sceneManager.updateObjectScale(obj->getId(), scale);
        }
    }

    ImGui::End();
}

void ApplayModernRoundedStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Corner rounding 8px
    style.WindowRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 8.0f;

	// Padding and spacing
    style.WindowPadding = ImVec2(16, 16);
    style.FramePadding = ImVec2(12, 8);
    style.ItemSpacing = ImVec2(12, 8);
    style.ItemInnerSpacing = ImVec2(8, 6);
    style.IndentSpacing = 25.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 12.0f;

    // Borders
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    // Ui pannel - grigio scuro
    colors[ImGuiCol_ChildBg] = ImVec4(0.235f, 0.235f, 0.255f, 1.0f); // rgb(60, 60, 65)

	// Top bar - griuo scuro
    colors[ImGuiCol_TitleBg] = ImVec4(0.176f, 0.176f, 0.196f, 1.0f); // rgb(45, 45, 50)
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.176f, 0.176f, 0.196f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.176f, 0.176f, 0.196f, 1.0f);

    // Button - viola pastello
    colors[ImGuiCol_Button] = ImVec4(0.627f, 0.549f, 0.784f, 0.40f); // rgba(160, 140, 200, 0.4)
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.627f, 0.549f, 0.784f, 0.70f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.627f, 0.549f, 0.784f, 1.00f);

    // Borders - grigio tonalità media
    colors[ImGuiCol_Border] = ImVec4(0.314f, 0.314f, 0.333f, 0.50f); // rgba(80, 80, 85, 0.5)
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Text
    colors[ImGuiCol_Text] = ImVec4(0.902f, 0.902f, 0.902f, 1.0f); // rgb(230, 230, 230)
    colors[ImGuiCol_TextDisabled] = ImVec4(0.627f, 0.627f, 0.647f, 1.0f); // rgb(160, 160, 165)
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.627f, 0.549f, 0.784f, 0.35f);

    // header/collapsing bar
    colors[ImGuiCol_Header] = ImVec4(0.627f, 0.549f, 0.784f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.627f, 0.549f, 0.784f, 0.50f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.627f, 0.549f, 0.784f, 0.70f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.098f, 0.098f, 0.118f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.314f, 0.314f, 0.333f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.392f, 0.392f, 0.412f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.510f, 0.510f, 0.529f, 1.0f);

	// Checkbox
    colors[ImGuiCol_CheckMark] = ImVec4(0.627f, 0.549f, 0.784f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.176f, 0.176f, 0.196f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.627f, 0.549f, 0.784f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.627f, 0.549f, 0.784f, 0.67f);

    // Slider
    colors[ImGuiCol_SliderGrab] = ImVec4(0.627f, 0.549f, 0.784f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.706f, 0.627f, 0.863f, 1.0f);

    // Tab
    colors[ImGuiCol_Tab] = ImVec4(0.235f, 0.235f, 0.255f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.627f, 0.549f, 0.784f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.549f, 0.471f, 0.706f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.176f, 0.176f, 0.196f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.314f, 0.314f, 0.333f, 1.0f);

	// Window background
    colors[ImGuiCol_WindowBg] = ImVec4(0.176f, 0.176f, 0.196f, 0.94f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.118f, 0.118f, 0.137f, 0.94f);

    // Resize grip
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.627f, 0.549f, 0.784f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.627f, 0.549f, 0.784f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.627f, 0.549f, 0.784f, 0.95f);

    // Separator
    colors[ImGuiCol_Separator] = ImVec4(0.314f, 0.314f, 0.333f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.392f, 0.392f, 0.412f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.510f, 0.510f, 0.529f, 1.0f);
}
