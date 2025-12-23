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
#include <cassert>

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
void renderShadowFloor(const glm::mat4& view, const glm::mat4& projection);
void renderSectionSceneObjects();
void processMousePicking(int x, int y);
void handlePickingResult(const glm::vec3& idColor);
void resetCameraView();
bool isPositionOccupied(const glm::vec3& position, float radius, const SceneManager& sceneManager);
glm::vec3 calculateSpawnPosition(const std::string& modelName, const glm::vec3& cameraTarget, const SceneManager& sceneManager);
void renderImGuizmo(const glm::mat4& view, const glm::mat4& projection);
void openModelFile();
void openImageFile();
void uiStyle();
void imGuizmoStyle();

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
static const glm::vec3 GRID_COLOR = glm::vec3(0.35f, 0.35f, 0.38f);
static const glm::vec3 GRID_X_AXIS_COLOR = glm::vec3(0.85f, 0.35f, 0.35f);
static const glm::vec3 GRID_Z_AXIS_COLOR = glm::vec3(0.35f, 0.50f, 0.85f);
static constexpr int gFiniteModeMaxObjects = 10;
static constexpr float minHeight = 0.5f;
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
    glm::vec3 color{ 1.0f, 0.95f, 0.8f };
    glm::vec3 gradientStart{ 1.0f, 0.9f, 0.6f };
    glm::vec3 gradientEnd{ 1.0f, 0.5f, 0.2f };
    bool useGradient = true;
    bool enabled = true;
    bool enablePlanarShadows = true;
    glm::vec4 shadowColor{ 0.0f, 0.0f, 0.0f, 0.35f };
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
static constexpr float gFloorHeight = -0.075f;
static bool gEnableShadowMap = true;
static constexpr unsigned int gShadowFloorObjectNumericId = 0x00FFFFFC;

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
struct ErrorDialog {
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
        title.clear();
        message.clear();
        details.clear();
    }
};

static SuccessDialog successDialog;

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
        exportFolder.clear();
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

struct ShadowMapData {
    static constexpr size_t ShadowMapResolution = 4096;

    GLuint Shader;
    GLuint Image;
    GLuint Framebuffer;
    glm::mat4x4 ViewProjection;
};
ShadowMapData gShadowMapData;

GLuint gPlanarFloorShader = 0;

struct UiWindowConfig {
    const char* id;
    ImVec2      defaultPos;
    ImVec2      defaultSize;
    bool        center;
    bool        movable;
    bool        useAlways;
    ImGuiWindowFlags extraFlags;
};

// Helper centralizzato per configurare finestra UI
static ImGuiWindowFlags BeginConfiguredWindow(const UiWindowConfig& cfg) {
    ImGuiCond cond = cfg.useAlways ? ImGuiCond_Always : ImGuiCond_FirstUseEver;

    if (cfg.center) {
        ImVec2 vpCenter = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(vpCenter, cond, ImVec2(0.5f, 0.5f));
    }
    else if (cfg.defaultPos.x > 0.0f || cfg.defaultPos.y > 0.0f) {
        ImGui::SetNextWindowPos(cfg.defaultPos, cond);
    }

    if (cfg.defaultSize.x > 0.0f && cfg.defaultSize.y > 0.0f) {
        ImGui::SetNextWindowSize(cfg.defaultSize, cond);
    }

    ImGuiWindowFlags flags = cfg.extraFlags;
    if (!cfg.movable) {
        flags |= ImGuiWindowFlags_NoMove;
    }

    return flags;
}

static bool ProjectToScreen(const glm::vec3& p, const glm::mat4& view, const glm::mat4& proj, ImVec2& outScreen) {
    glm::vec4 clip = proj * view * glm::vec4(p, 1.0f);
    if (clip.w <= 0.0f) return false;
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f || ndc.z < -1.0f || ndc.z > 1.0f) {
        return false;
    }
    float x = (ndc.x * 0.5f + 0.5f) * static_cast<float>(gFramebufferWidth);
    float y = ((-ndc.y) * 0.5f + 0.5f) * static_cast<float>(gFramebufferHeight);
    outScreen = ImVec2(x, y);
    return true;
}

static void drawLightDirectionOverlay(const glm::mat4& view, const glm::mat4& projection) {
    if (!gDirLight.enabled) return;

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

    glm::vec3 end;
    bool hasIntersection = false;
    {
        float denom = dir.y;
        if (std::abs(denom) > 1e-6f) {
            float tHit = (gFloorHeight - gDirLight.position.y) / denom;
            if (tHit >= 0.0f) {
                end = gDirLight.position + dir * tHit;
                hasIntersection = true;
            }
        }
    }

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

static inline glm::vec3 ClampRenderablePosition(const glm::vec3& p) {
    glm::vec3 clamped = clampToFiniteSpace(p);
    clamped.y = (std::max)(clamped.y, minHeight);
    return clamped;
}

static float getObjectEffectiveRadius(const SceneObject& obj) {
    float baseRadius = 0.5f;
    glm::vec3 s = obj.getScale();
    return baseRadius * (std::max)(s.x, s.z);
}

// Target directional light
static constexpr unsigned int gDirLightTargetObjectNumericId = 0x00FFFFFD;
static constexpr float kTargetLift = 0.02f;

static inline glm::vec3 ClampTargetOnFloor(const glm::vec3& p) {
    glm::vec3 c = p;
    c.y = gFloorHeight + kTargetLift;
    return clampToFiniteSpace(c);
}

// Controlla se una posizione è occupata
bool isPositionOccupied(const glm::vec3& position, float newObjRadius, const SceneManager& manager) {
    for (const auto& obj : manager.getObjects()) {
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
    float yOffset = 0.0f;
    if (modelName == "Cube" || modelName == "Sphere") yOffset = 0.5f;
    else if (modelName == "Pyramid") yOffset = 0.0f;
    else yOffset = 1.0f;

    float newObjRadius = 0.75f;

    if (gInfiniteGrid) {
        glm::vec3 pos = manager.findValidSpawnPosition(mouseControl.camPos, cameraTarget, yOffset, newObjRadius);
        pos.y = (std::max)(pos.y, minHeight);
        return pos;
    }

    float halfExtent = gGridHalfSize;
    glm::vec3 pos = manager.findValidSpawnPositionFinite(
        mouseControl.camPos, cameraTarget, yOffset, newObjRadius, halfExtent, gGridBoundaryMargin);
    pos.y = (std::max)(pos.y, minHeight);
    return pos;

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

    return best;
}
// Helper per recuperare oggetto per ID
static std::shared_ptr<SceneObject> findObjectById(unsigned int id) {
    for (const auto& o : sceneManager.getObjects()) {
        if (o->getId() == id) return o;
    }
    return nullptr;
}

// Callback framebuffer
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (width > 0 && height > 0) {
        gFramebufferWidth = width;
        gFramebufferHeight = height;

        int ww, wh;
        glfwGetWindowSize(window, &ww, &wh);
        gWindowWidthLogical = ww;
        gWindowHeightLogical = wh;

        Logger::Get().LogWindowResize(gWindowWidthLogical, gWindowHeightLogical, gFramebufferWidth, gFramebufferHeight);

        glViewport(0, 0, gFramebufferWidth, gFramebufferHeight);

        if (pickingBuffer.isInitialized()) {
            pickingBuffer.resize(gFramebufferWidth, gFramebufferHeight);
        }
    }
}

// Calcolo posizione camera
void updateCameraPosition() {
    float radXZ = mouseControl.cameraDistance * cosf(glm::radians(mouseControl.orbitalAngleY));
    float camX = radXZ * sinf(glm::radians(mouseControl.orbitalAngleX));
    float camY = mouseControl.cameraDistance * sinf(glm::radians(mouseControl.orbitalAngleY));
    float camZ = radXZ * cosf(glm::radians(mouseControl.orbitalAngleX));
    mouseControl.camPos = mouseControl.cameraTarget + glm::vec3(camX, camY + mouseControl.cameraHeight / 2, camZ);
    if (mouseControl.camPos.y < 0.5f) mouseControl.camPos.y = 0.5f;

    float extent = (std::max)(20.0f, mouseControl.cameraDistance * 5.0f);
    sceneManager.updateObjectScale(gShadowFloorObjectNumericId, glm::vec3(extent, 1.0f, extent));

    if (auto floorObj = findObjectById(gShadowFloorObjectNumericId)) {
        glm::vec3 p = floorObj->getPosition();
        p.y = gFloorHeight - 0.01f;
        sceneManager.updateObjectPosition(gShadowFloorObjectNumericId, p);
    }
}

void resetCameraView() {
    mouseControl.cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    mouseControl.cameraDistance = 3.0f;
    mouseControl.orbitalAngleX = 0.0f;
    mouseControl.orbitalAngleY = 20.0f;
    updateCameraPosition();
}

// Helper rendering
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

static inline void ConvertCursorToFramebuffer(GLFWwindow* /*window*/, double cursorX, double cursorY, int& outX, int& outY) {
    int ww = gWindowWidthLogical;
    int wh = gWindowHeightLogical;
    int fbw = gFramebufferWidth;
    int fbh = gFramebufferHeight;

    double scaledX = cursorX * static_cast<double>(fbw) / static_cast<double>(ww);
    double scaledY = cursorY * static_cast<double>(fbh) / static_cast<double>(wh);

    outX = static_cast<int>(std::clamp(scaledX, 0.0, static_cast<double>(fbw - 1)));
    outY = static_cast<int>(std::clamp(scaledY, 0.0, static_cast<double>(fbh - 1)));
}

// Picking
void processMousePicking(int xWindow, int yWindow) {
    if (!pickingEnabled || !pickingBuffer.isInitialized()) return;

    int px, py;
    ConvertCursorToFramebuffer(glfwGetCurrentContext(), static_cast<double>(xWindow), static_cast<double>(yWindow), px, py);

    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);

    glDisable(GL_BLEND);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);

    pickingBuffer.bind();
    glViewport(0, 0, pickingBuffer.getWidth(), pickingBuffer.getHeight());
    pickingBuffer.clear();

    float aspect = static_cast<float>(pickingBuffer.getWidth()) / static_cast<float>(pickingBuffer.getHeight());
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(mouseControl.camPos, mouseControl.cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    glUseProgram(pickingShader);
    SetUniformMat4(pickingShader, "view", view);
    SetUniformMat4(pickingShader, "projection", projection);

    sceneManager.renderForPicking(pickingShader, view, projection);

    glFlush();
    glFinish();

    glm::vec3 idColor = pickingBuffer.readPixel(px, py);
    pickingBuffer.unbind();

    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);

    glm::vec3 q = QuantizeIdColor(idColor);
    handlePickingResult(q);
}

float lastClickTime = 0.0f;
const float doubleClickTimeThreshold = 0.3f;
unsigned int lastClickedObjectId = 0;

bool pinSelectedModelPanel = false;
ImVec2 selectedPanelSize = ImVec2(350, 0);

// Risultato picking
void handlePickingResult(const glm::vec3& idColor) {
    unsigned int pickedId = sceneManager.colorToId(idColor);

    if (pickedId == 0) {
        sceneManager.deselectAll();
        objectSelected = false;
        if (!pinSelectedModelPanel) showSelectedModelPanel = false;
        ImGuizmo::Enable(false);
        return;
    }

    if (pickedId == gShadowFloorObjectNumericId) {
        sceneManager.deselectAll();
        objectSelected = false;
        if (!pinSelectedModelPanel) showSelectedModelPanel = false;
        ImGuizmo::Enable(false);
        return;
    }

    sceneManager.selectObject(pickedId);

    bool isLight = (pickedId == gDirLightObjectNumericId);
    bool isLightTarget = (pickedId == gDirLightTargetObjectNumericId);
    objectSelected = !isLight;
    showSelectedModelPanel = !isLight;
    ImGuizmo::Enable(true);

    if (isLight || isLightTarget) {
        objectSelected = true;
        showSelectedModelPanel = false;
        ImGuizmo::Enable(true);
        return;
    }

    if (auto sel = sceneManager.getSelectedObject()) {
        displayedEulerAngles = glm::degrees(glm::eulerAngles(sel->getRotation()));
    }
}

// Mouse
void mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (objectSelected && (ImGuizmo::IsUsing() || ImGuizmo::IsOver())) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
            mouseControl.isOrbiting = true;
            mouseControl.isPressed = false;
            mouseControl.isPanning = false;
        }
        else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            mouseControl.isPanning = true;
            mouseControl.isOrbiting = false;
            mouseControl.isPressed = false;
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
                currentMoveAxis = MoveAxis::NONE;
            }

            lastMousePos = glm::vec3(static_cast<float>(xpos), static_cast<float>(ypos), 0.0f);
        }
        else {
            if (pickingEnabled) {
                processMousePicking(static_cast<int>(xpos), static_cast<int>(ypos));
            }
            mouseControl.isOrbiting = false;
            mouseControl.isPanning = false;
            mouseControl.isPressed = true;
        }

        mouseControl.lastX = static_cast<float>(xpos);
        mouseControl.lastY = static_cast<float>(ypos);
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        mouseControl.isPressed = false;
        mouseControl.isOrbiting = false;
        mouseControl.isPanning = false;
        objectMoving = false;
        currentMoveAxis = MoveAxis::NONE;
    }
}

void cursorPositionCallback(GLFWwindow* /*window*/, double xpos, double ypos) {
    float xoffset = static_cast<float>(xpos) - mouseControl.lastX;
    float yoffset = mouseControl.lastY - static_cast<float>(ypos);

    mouseControl.lastX = static_cast<float>(xpos);
    mouseControl.lastY = static_cast<float>(ypos);

    const float sensitivity = 0.3f;

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

        mouseControl.orbitalAngleX += xoffset * sensitivity;
        updateCameraPosition();
    }
    else if (mouseControl.isPanning) {
        const float panSpeed = 0.003f * mouseControl.cameraDistance;

        glm::vec3 viewDir = mouseControl.cameraTarget - mouseControl.camPos;
        glm::vec3 forwardOnPlane = glm::normalize(glm::vec3(viewDir.x, 0.0f, viewDir.z));
        glm::vec3 rightOnPlane = glm::normalize(glm::cross(forwardOnPlane, glm::vec3(0.0f, 1.0f, 0.0f)));

        mouseControl.cameraTarget -= rightOnPlane * xoffset * panSpeed;
        mouseControl.cameraTarget -= forwardOnPlane * yoffset * panSpeed;

        updateCameraPosition();
    }
}

// Scroll
void scrollCallback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) return;

    float zoomSpeed = 0.5f;
    mouseControl.cameraDistance -= static_cast<float>(yoffset) * zoomSpeed;

    if (mouseControl.cameraDistance < 0.5f) mouseControl.cameraDistance = 0.5f;
    if (mouseControl.cameraDistance > 25.0f) mouseControl.cameraDistance = 25.0f;

    updateCameraPosition();
}

// Picking init
void initializePickingSystem() {
    if (!pickingBuffer.initialize(gFramebufferWidth, gFramebufferHeight)) {
        Logger::Get().Log(Logger::Level::Error, Logger::Category::System, "Errore nell'inizializzazione del buffer di picking.");
        return;
    }

    pickingShader = LoadShader("Shaders/Picking.vert", "Shaders/Picking.frag");

    if (pickingShader == 0) {
        Logger::Get().LogShaderError("Picking", "Errore nel caricamento dello shader di picking.");
        return;
    }

    Logger::Get().LogShaderLoaded("Picking");
}

// Scene render
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
    SetUniformInt(shader, "uDirLight.enabled", gDirLight.enabled ? 1 : 0);
    SetUniformVec3(shader, "uDirLight.direction", lightDir);
    SetUniformVec3(shader, "uDirLight.color", gDirLight.color);

    SetUniformInt(shader, "uUseShadowMap", (gEnableShadowMap && gDirLight.enabled) ? 1 : 0);
    SetUniformInt(shader, "uShadowMapSize", gShadowMapData.ShadowMapResolution);
    SetUniformMat4(shader, "lightSpaceMatrix", gShadowMapData.ViewProjection);

    static constexpr uint32_t shadowMapUnit = 1;
    glActiveTexture(GL_TEXTURE0 + shadowMapUnit);
    glBindTexture(GL_TEXTURE_2D, gShadowMapData.Image);
    glUniform1i(glGetUniformLocation(shader, "uShadowMap"), shadowMapUnit);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::vector<std::shared_ptr<SceneObject>> list = sceneManager.getObjects();
    std::stable_sort(list.begin(), list.end(), [&](const auto& a, const auto& b) {
        bool aSys = sceneManager.isSystemId(a->getId());
        bool bSys = sceneManager.isSystemId(b->getId());
        if (aSys != bSys) return aSys;
        return a->getId() < b->getId();
        });

    for (const auto& obj : list) {
        if (obj->getId() == gShadowFloorObjectNumericId) {
            continue;
        }

        glm::mat4 modelMatrix = obj->getModelMatrix();
        SetUniformMat4(shader, "model", modelMatrix);

        bool isLightIcon = (obj->getId() == gDirLightObjectNumericId);
        bool isLightTarget = (obj->getId() == gDirLightTargetObjectNumericId);
		bool isFloor = (obj->getId() == gShadowFloorObjectNumericId);

        SetUniformInt(shader, "uLightIcon", isLightIcon ? 1 : 0);
        SetUniformInt(shader, "uLightGradientEnabled", (isLightIcon && gDirLight.useGradient) ? 1 : 0);
        SetUniformVec3(shader, "uLightGradientStart", gDirLight.gradientStart);
        SetUniformVec3(shader, "uLightGradientEnd", gDirLight.gradientEnd);
        SetUniformVec3(shader, "uLightEmitColor", gDirLight.color);

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
            GLuint overrideTexID = obj->getOverrideTextureID();
            if (overrideTexID != 0 && glIsTexture(overrideTexID)) {
                glBindTexture(GL_TEXTURE_2D, overrideTexID);
                SetUniformInt(shader, "useOverrideColor", 0);
                SetUniformInt(shader, "useTexture", 1);
            }
            else {
                glBindTexture(GL_TEXTURE_2D, TextureManager::getInstance().getDefaultTexture());
                SetUniformInt(shader, "useOverrideColor", 0);
                SetUniformInt(shader, "useTexture", 0);
            }
        }
        else if (obj->hasOverrideColor()) {
            glBindTexture(GL_TEXTURE_2D, TextureManager::getInstance().getDefaultTexture());
            SetUniformInt(shader, "useOverrideColor", 1);
            SetUniformVec4(shader, "overrideColor", obj->getOverrideColor());
            SetUniformInt(shader, "useTexture", 0);
        }
        else if (model->hasTexture()) {
            GLuint modelTexID = model->getTextureID();
            if (modelTexID != 0 && glIsTexture(modelTexID)) {
                glBindTexture(GL_TEXTURE_2D, modelTexID);
                SetUniformInt(shader, "useOverrideColor", 0);
                SetUniformInt(shader, "useTexture", 1);
            }
            else {
                glBindTexture(GL_TEXTURE_2D, TextureManager::getInstance().getDefaultTexture());
                SetUniformInt(shader, "useOverrideColor", 0);
                SetUniformInt(shader, "useTexture", 0);
            }
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

static void renderShadowFloor(const glm::mat4& view, const glm::mat4& projection) {
    auto floorObj = findObjectById(gShadowFloorObjectNumericId);
    if (!floorObj) {
        return;
    }

    if (!gEnableShadowMap || !gDirLight.enabled || !gDirLight.enablePlanarShadows) {
        return;
    }

    glUseProgram(gPlanarFloorShader);

    glm::mat4 model = floorObj->getModelMatrix();

    SetUniformMat4(gPlanarFloorShader, "model", model);
    SetUniformMat4(gPlanarFloorShader, "view", view);
    SetUniformMat4(gPlanarFloorShader, "projection", projection);
    SetUniformMat4(gPlanarFloorShader, "lightSpaceMatrix", gShadowMapData.ViewProjection);

    SetUniformFloat(gPlanarFloorShader, "uFloorHeight", gFloorHeight);
    SetUniformInt(gPlanarFloorShader, "uUseShadowMap", 1);
    SetUniformInt(gPlanarFloorShader, "uShadowMapSize", static_cast<int>(ShadowMapData::ShadowMapResolution));
    SetUniformVec3(gPlanarFloorShader, "uBackgroundColor", glm::vec3(0.18f, 0.18f, 0.20f));

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
    SetUniformVec3(gPlanarFloorShader, "uLightDir", lightDir);

    SetUniformFloat(gPlanarFloorShader, "uShadowStrength", 1.0f);

    static constexpr GLuint shadowMapUnit = 3;
    glActiveTexture(GL_TEXTURE0 + shadowMapUnit);
    glBindTexture(GL_TEXTURE_2D, gShadowMapData.Image);

    GLint loc = glGetUniformLocation(gPlanarFloorShader, "uShadowMap");
    glUniform1i(loc, shadowMapUnit);

    glDisable(GL_BLEND);

    if (auto modelPtr = floorObj->getModel()) {
        modelPtr->render();
    }

    glUseProgram(0);
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
            pfd->SetTitle(L"Choose where to save the scene");

            hr = pfd->Show(NULL);
            if (SUCCEEDED(hr)) {
                IShellItem* psi;
                hr = pfd->GetResult(&psi);
                if (SUCCEEDED(hr)) {
                    PWSTR pszPath;
                    hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                    if (SUCCEEDED(hr)) {
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

#endif
}

// Dialog informativo limite oggetti in modalità finita
void renderFiniteLimitDialog() {
    if (!showFiniteLimitDialog || !gFiniteLimitReached) return;

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(500, 150), ImVec2(800, 400));

    ImGui::OpenPopup("Object Limit Reached");

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(25, 20));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.75f, 0.55f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.85f, 0.65f, 0.20f, 1.0f));

    if (ImGui::BeginPopupModal("Object Limit Reached", &showFiniteLimitDialog,
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings)) {

        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x - 10);

        ImGui::TextColored(ImVec4(0.85f, 0.70f, 0.35f, 1.0f), "HEADS UP");
        ImGui::TextWrapped("You've hit the max of %d objects.", gFiniteModeMaxObjects);
        ImGui::TextWrapped("Switch to 'Infinite' mode in Grid Settings to add more.");

        ImGui::PopTextWrapPos();

        ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        float buttonWidth = 140.0f;
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((avail - buttonWidth) * 0.5f);

        if (ImGui::Button("Got it", ImVec2(buttonWidth, 32)) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape) ||
            ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            showFiniteLimitDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

// Texture
void openTextureFile() {
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
            if (auto selectedObj = sceneManager.getSelectedObject()) {
                selectedObj->setOverrideTexture(textureID);
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

// Model loading
void openModelFile() {
    if (shouldBlockAdd()) {
        showFiniteLimitDialog = true;
        return;
    }

    glm::vec3 spawnPos = sceneManager.findValidSpawnPosition(
        mouseControl.camPos,
        mouseControl.cameraTarget,
        0.5f, 1.0f
    );

    auto progressCallback = [](float /*progress*/, const std::string& /*message*/) {};

    bool success = ModelLoader::openModelFileAdvanced(
        modelManager, sceneManager,
        mouseControl.camPos, mouseControl.cameraTarget,
        objectSelected, showSelectedModelPanel,
        ModelLoader::LoadMode::SEPARATE_MESHES,
        progressCallback
    );

    if (!success) {
        errorDialog.show = true;
        errorDialog.title = "Model Loading Error";
        errorDialog.message = "Failed to load model.";
    }
}

// Image file
void openImageFile() {
    if (shouldBlockAdd()) {
        showFiniteLimitDialog = true;
        return;
    }

    glm::vec3 spawnPos = sceneManager.findValidSpawnPosition(
        mouseControl.camPos,
        mouseControl.cameraTarget,
        0.5f,
        1.0f
    );

    spawnPos.y = (std::max)(spawnPos.y, minHeight);

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

    if (!success && !errorMessage.empty()) {
        errorDialog.show = true;
        errorDialog.title = "Image Loading Error";
        errorDialog.message = errorMessage;
    }
}

// Export dialog
void renderExportFormatDialog() {
    if (!exportFormatDialog.show) return;

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_Always);

    ImGui::OpenPopup("Export 3D Scene");

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.16f, 0.16f, 0.18f, 1.0f));

    if (ImGui::BeginPopupModal("Export 3D Scene", &exportFormatDialog.show,
        ImGuiWindowFlags_NoResize)) {

        ImGui::TextWrapped("Scegli il formato di export per la tua scena 3D.");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Formato File:");
        ImGui::Spacing();

        for (size_t i = 0; i < exportFormatDialog.formats.size(); i++) {
            const auto& fmt = exportFormatDialog.formats[i];
            bool isSelected = (i == static_cast<size_t>(exportFormatDialog.selectedFormatIndex));

            if (ImGui::RadioButton(fmt.name.c_str(), isSelected)) {
                exportFormatDialog.selectedFormatIndex = static_cast<int>(i);
            }

            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", fmt.extension.c_str());

            ImGui::Indent(30);
            ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x - 10);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", fmt.description.c_str());
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "%s", fmt.useCases.c_str());
            ImGui::PopTextWrapPos();
            ImGui::Unindent(30);

            ImGui::Spacing();
        }

        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Opzioni:");

        if (exportFormatDialog.selectedFormatIndex < static_cast<int>(exportFormatDialog.formats.size())) {
            const auto& selectedFmt = exportFormatDialog.formats[exportFormatDialog.selectedFormatIndex];

            bool canEmbed = selectedFmt.supportsTextures && selectedFmt.supportsEmbeddedTextures;
            bool canCopy = selectedFmt.supportsTextures;

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
                exportFormatDialog.copyTextures = true;
                ImGui::TextDisabled("Le texture saranno salvate in 'textures/' accanto al file.");
            }
            if (!canCopy) ImGui::EndDisabled();

            bool embed = exportFormatDialog.embedTextures &&
                selectedFmt.supportsEmbeddedTextures &&
                selectedFmt.supportsTextures;
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

        float buttonWidth = (ImGui::GetContentRegionAvail().x - 10) / 2;

        if (ImGui::Button("Annulla", ImVec2(buttonWidth, 35))) {
            exportFormatDialog.reset();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.28f, 0.32f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.33f, 0.38f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.23f, 0.27f, 1.0f));

        if (ImGui::Button("Esporta", ImVec2(buttonWidth, 35))) {
            const auto& selectedFmt = exportFormatDialog.formats[exportFormatDialog.selectedFormatIndex];

            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");

            std::string filename = "Scene_" + ss.str() + selectedFmt.extension;
            std::string outputPath = exportFormatDialog.exportFolder + "/" + filename;

            Logger::Get().LogExportStarted(selectedFmt.name, outputPath);

            bool embed = exportFormatDialog.embedTextures &&
                selectedFmt.supportsEmbeddedTextures &&
                selectedFmt.supportsTextures;
            bool copy = exportFormatDialog.copyTextures &&
                selectedFmt.supportsTextures && !embed;

            auto result = AssimpSceneExporter::exportScene(
                sceneManager,
                modelManager,
                outputPath,
                selectedFmt.id,
                copy,
                embed
            );

            if (result.success) {
                Logger::Get().LogExportCompleted(
                    outputPath,
                    result.fileSize,
                    result.exportTimeSeconds,
                    result.totalVertices,
                    result.totalFaces,
                    result.totalMaterials
                );
                successDialog.show = true;
                successDialog.title = "Export Successful";
                successDialog.message = "Scena esportata con successo!";

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
                    }
                    else if (result.externalTexturesCopied) {
                        details << "Texture: " << result.texturesCopiedCount << " (copiate in /textures)\n";
                    }
                    else {
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
                Logger::Get().LogExportError(outputPath, result.errorMessage);

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

// Aggiorna stato limite in modalità finita
void updateFiniteLimitState() {
    static bool previousReached = false;

    if (gInfiniteGrid) {
        gFiniteLimitReached = false;
        showFiniteLimitDialog = false;
        previousReached = false;
        return;
    }

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

// Errore cambio grid mode
static void renderGridModeSwitchErrorDialog() {
    if (!showGridModeSwitchErrorPopup) return;

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::OpenPopup("Can't Switch Grid Mode");

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22, 18));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.75f, 0.25f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.85f, 0.30f, 0.25f, 1.0f));

    if (ImGui::BeginPopupModal("Can't Switch Grid Mode", &showGridModeSwitchErrorPopup,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {

        ImGui::TextColored(ImVec4(0.90f, 0.50f, 0.35f, 1.0f), "CAN'T SWITCH TO FINITE MODE");
        ImGui::TextDisabled("Tip: remove some objects or move them inside the boundary.");

        ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        float buttonWidth = 140.0f;
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((avail - buttonWidth) * 0.5f);

        if (ImGui::Button("OK", ImVec2(buttonWidth, 32)) ||
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

// Grid Mode
void renderSectionGridMode() {
    if (ImGui::CollapsingHeader("Grid Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
        int currentMode = gInfiniteGrid ? 1 : 0;
        bool clickedFinite = ImGui::RadioButton("Finite", currentMode == 0);
        ImGui::SameLine();
        bool clickedInfinite = ImGui::RadioButton("Infinite", currentMode == 1);

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
            ImGui::TextWrapped("SPACE LIMITED");
            ImGui::PopStyleColor();

            if (gFiniteLimitReached) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.3f, 0.3f, 1.0f));
                ImGui::TextWrapped("Limit reached");
                ImGui::PopStyleColor();
            }
        }
    }
}

// Scene Objects
void renderSectionSceneObjects() {

    bool disabled = gFiniteLimitReached && !gInfiniteGrid;

    if (ImGui::CollapsingHeader("Scene Objects", ImGuiTreeNodeFlags_DefaultOpen)) {


        if (ImGui::Button("Export Scene", ImVec2(-1, 30))) {
            std::string folderPath = openFolderDialog();
            if (!folderPath.empty()) {
                exportFormatDialog.open(folderPath);
            }
        }

        ImGui::Spacing();
        ImGui::Text("Import:");

        if (disabled) ImGui::BeginDisabled();

        if (ImGui::Button("Load 3D Model", ImVec2(-1, 28)) && !disabled) {
			ImGui::OpenPopup("Choose Load Mode");
        }

        if (ImGui::Button("Load Image", ImVec2(-1, 28)) && !disabled) {
            openImageFile();
        }
        if (disabled) ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Text("Add Built-in:");

        std::vector<std::string> modelNames = modelManager.getModelNames();
        modelNames.erase(std::remove_if(modelNames.begin(), modelNames.end(), [](const std::string& name) 
            { return name == "ImagePlane" || name == "ShadowFloor";}), modelNames.end());

        static int selectedModelIndex = 0;

        if (!modelNames.empty()) {
            ImGui::PushItemWidth(-1);
            ImGui::Combo("##ModelType", &selectedModelIndex,
                [](void* data, int idx, const char** out_text) {
                    auto& names = *static_cast<std::vector<std::string>*>(data);
                    if (idx >= 0 && idx < static_cast<int>(names.size())) {
                        *out_text = names[idx].c_str();
                        return true;
                    }
                    return false;
                },
                &modelNames,
                static_cast<int>(modelNames.size()));
            ImGui::PopItemWidth();

            if (disabled) ImGui::BeginDisabled();

            if (ImGui::Button("Add Object", ImVec2(-1, 28)) &&
                !disabled && selectedModelIndex < static_cast<int>(modelNames.size())) {
                const std::string& modelName = modelNames[selectedModelIndex];
                glm::vec3 spawnPos = calculateSpawnPosition(modelName, mouseControl.cameraTarget, sceneManager);

                if (!gInfiniteGrid) {
                    glm::vec3 clamped = clampToFiniteSpace(spawnPos);
                    if (clamped != spawnPos) {
                        gLastPlacementClamped = true;
                        spawnPos = clamped;
                    }
                }
                spawnPos.y = (std::max)(spawnPos.y, minHeight);

                if (auto newObj = sceneManager.addObject(modelName, spawnPos)) {
                    sceneManager.applyDefaultName(newObj->getId(), modelName);
                    sceneManager.selectObject(newObj->getId());
                    objectSelected = true;
                    showSelectedModelPanel = true;

                    Logger::Get().Log(Logger::Level::Info, Logger::Category::Scene,
                        "Built-in object '" + modelName + "' added to the scene.");
                } else {
                    Logger::Get().Log(Logger::Level::Warn, Logger::Category::Scene,
                        "Failed to add built-in object '" + modelName + "'.");
                }
            }
            if (disabled) ImGui::EndDisabled();
        }

        ImGui::Spacing();
        ImGui::Text("Scene:");

        float listHeight = ImGui::GetContentRegionAvail().y - 60;
        if (listHeight < 100.0f) listHeight = 100.0f;

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
                if (ImGui::InputText("##rename_user", renameBuffer, sizeof(renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
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

            bool renamingThis = isRenaming && renamingId == obj->getId();
            bool isLight = (obj->getId() == gDirLightObjectNumericId);
            bool isLightTarget = (obj->getId() == gDirLightTargetObjectNumericId);
            bool isFloor = (obj->getId() == gShadowFloorObjectNumericId);

            if (renamingThis && (isLight || isLightTarget)) {
                ImGui::PushID(static_cast<int>(obj->getId()));
                if (ImGui::InputText("##rename_sys", renameBuffer, sizeof(renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
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
                if ((isLight || isLightTarget) && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    isRenaming = true;
                    renamingId = obj->getId();
                    strncpy(renameBuffer, obj->getName().c_str(), sizeof(renameBuffer) - 1);
                }
            }

            if (!renamingThis && ImGui::IsItemClicked()) {
                sceneManager.selectObject(obj->getId());
                objectSelected = !(isLight || isLightTarget || isFloor);
                showSelectedModelPanel = !(isLight || isLightTarget || isFloor);
                if (isLight && !pinDirectionalLightWindow) {
                    showDirectionalLightWindow = true;
                }
            }

            if (ImGui::BeginPopupContextItem()) {
                if ((isLight || isLightTarget) && ImGui::MenuItem("Rename")) {
                    isRenaming = true;
                    renamingId = obj->getId();
                    strncpy(renameBuffer, obj->getName().c_str(), sizeof(renameBuffer) - 1);
                }
                ImGui::TextDisabled("System object");
                ImGui::EndPopup();
            }
        }

        ImGui::EndChild();
        ImGui::Checkbox("Enable Picking", &pickingEnabled);

        {
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal("Choose Load Mode", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("How should I load this model?");
                ImGui::Separator();

                if (ImGui::Button("Single Object", ImVec2(260, 0))) {
                    ModelLoader::openModelFileAdvanced(
                        modelManager, sceneManager,
                        mouseControl.camPos, mouseControl.cameraTarget,
                        objectSelected, showSelectedModelPanel,
                        ModelLoader::LoadMode::SINGLE_OBJECT);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::TextWrapped("Everything as one object");
                ImGui::Spacing();

                if (ImGui::Button("Separate Meshes", ImVec2(260, 0))) {
                    ModelLoader::openModelFileAdvanced(
                        modelManager, sceneManager,
                        mouseControl.camPos, mouseControl.cameraTarget,
                        objectSelected, showSelectedModelPanel,
                        ModelLoader::LoadMode::SEPARATE_MESHES);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::TextWrapped("Each mesh is its own object");
                ImGui::Separator();

                if (ImGui::Button("Cancel", ImVec2(260, 0))) {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }
    }
}

// Camera
void renderSectionCamera() {
    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::Text("Distance:");
        ImGui::PushItemWidth(-1);
        if (ImGui::SliderFloat("##CamDist", &mouseControl.cameraDistance, 0.5f, 25.0f)) {
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

        ImGui::Spacing();

        ImGui::Checkbox("Light##DirLight", &gDirLight.enabled);
        ImGui::SameLine();
        ImGui::Checkbox("Shadows", &gDirLight.enablePlanarShadows);
    }
}

// Actions
void renderSectionActions() {
    ImGui::Text("Tools:");

    if (ImGui::Button("Help", ImVec2(-1, 28))) {
        shoeHelpWindow = !shoeHelpWindow;
    }


    if (ImGui::Button("Console", ImVec2(-1, 28))) {
        gShowConsole = !gShowConsole;
    }
}

static void OpenUrlInBrowser(const char* url) {
#ifdef _WIN32
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
#endif
}

int main() {
    Logger::Get().Log(Logger::Level::Trace, Logger::Category::System,
        "Working directory: " + std::filesystem::current_path().string());

    Window win;
    if (!win.initialize(gFramebufferWidth, gFramebufferHeight, "3D Modeler")) {
        Logger::Get().Log(Logger::Level::Error, Logger::Category::Window, "Errore inizializzazione finestra");
        return -1;
    }

    GLFWimage icon;
    int iconWidth, iconHeight, iconChannels;
    unsigned char* iconData = stbi_load("Assets/Images/logo.png", &iconWidth, &iconHeight, &iconChannels, 4);
    if (iconData) {
        icon.width = iconWidth;
        icon.height = iconHeight;
        icon.pixels = iconData;
        glfwSetWindowIcon(win.getGLFWwindow(), 1, &icon);
        stbi_image_free(iconData);
        Logger::Get().Log(Logger::Level::Trace, Logger::Category::System, "Icona finestra caricata");
    }
    else {
        Logger::Get().Log(Logger::Level::Warn, Logger::Category::System, "Impossibile caricare icona finestra");
    }

    Logger& logger = Logger::Get();
    logger.SetGlfwWindow(win.getGLFWwindow());
    logger.SetAppVersion("0.1.0");
    logger.SetNotionFormUrl("");
    logger.Log(Logger::Level::Info, Logger::Category::App, "Applicazione avviata.");

    glfwSetMouseButtonCallback(win.getGLFWwindow(), mouseButtonCallback);
    glfwSetCursorPosCallback(win.getGLFWwindow(), cursorPositionCallback);
    glfwSetScrollCallback(win.getGLFWwindow(), scrollCallback);
    glfwSetFramebufferSizeCallback(win.getGLFWwindow(), framebufferSizeCallback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(win.getGLFWwindow(), true);
    ImGui_ImplOpenGL3_Init("#version 450");

    uiStyle();
    imGuizmoStyle();

#ifdef _WIN32
    {
        HWND hwnd = glfwGetWin32Window(win.getGLFWwindow());
        if (hwnd) {
            BOOL useDarkMode = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

            COLORREF titleBarColor = RGB(30, 30, 35);
            COLORREF titleTextColor = RGB(200, 200, 205);
            COLORREF borderColor = RGB(45, 45, 50);

            DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &titleBarColor, sizeof(titleBarColor));
            DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &titleTextColor, sizeof(titleTextColor));
            DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
        }
    }
#endif

    logger.Log(Logger::Level::Info, Logger::Category::System, "Rilevate informazioni GPU.");

    GLuint shader = LoadShader("Shaders/Object.vert", "Shaders/Object.frag");
    if (shader == 0) {
        logger.LogShaderError("Object", "Errore nel caricamento degli shader.");
        return -1;
    }
    logger.LogShaderLoaded("Object");

    GLuint gridShader = LoadShader("Shaders/Grid.vert", "Shaders/Grid.frag");
    if (gridShader == 0) {
        logger.LogShaderError("Grid", "Errore nel caricamento degli shader.");
        return -1;
    }
    logger.LogShaderLoaded("Grid");

    GLuint dirShadowDepthShader = LoadShader("Shaders/DirShadowDepth.vert", "Shaders/DirShadowDepth.frag");
    if (dirShadowDepthShader == 0) {
        logger.LogShaderError("DirShadowDepth", "Shader depth luce non caricato.");
    }
    else {
        logger.LogShaderLoaded("DirShadowDepth");
    }

    gPlanarFloorShader = LoadShader("Shaders/PlanarShadowFloor.vert", "Shaders/PlanarShadowFloor.frag");
    if (gPlanarFloorShader == 0) {
        logger.LogShaderError("PlanarShadowFloor", "Errore nel caricamento degli shader del floor.");
        return -1;
    }
    logger.LogShaderLoaded("PlanarShadowFloor");

    modelManager.registerModel(std::make_shared<CubeModel>());
    modelManager.registerModel(std::make_shared<SphereModel>());
    modelManager.registerModel(std::make_shared<PyramidModel>());
    modelManager.registerModel(std::make_shared<ImagePlaneModel>());
    modelManager.registerModel(std::make_shared<ShadowFloor>());

    modelManager.initializeModels();
    logger.Log(Logger::Level::Info, Logger::Category::Model, "Modelli base registrati e inizializzati.");

    initializePickingSystem();
    logger.Log(Logger::Level::Info, Logger::Category::System, "Sistema di picking inizializzato.");

    Grid grid;
    grid.initialize();
    logger.Log(Logger::Level::Info, Logger::Category::Grid, "Griglia inizializzata.");

    glm::vec3 lightInitialPos = gDirLight.position;
    auto lightObj = sceneManager.addSystemObject("Sphere", lightInitialPos, gDirLightObjectNumericId, "Directional Light");
    if (lightObj) {
        sceneManager.updateObjectScale(gDirLightObjectNumericId, glm::vec3(0.15f));
        logger.Log(Logger::Level::Info, Logger::Category::Light, "Oggetto luce direzionale aggiunto.");
    }

    {
        gDirLight.target = ClampTargetOnFloor(gDirLight.target);
        auto targetObj = sceneManager.addSystemObject("ImagePlane", gDirLight.target, gDirLightTargetObjectNumericId, "Light Target");
        if (targetObj) {
            sceneManager.updateObjectScale(gDirLightTargetObjectNumericId, glm::vec3(0.15f, 0.15f, 0.15f));
            glm::quat rot = glm::quat(glm::radians(glm::vec3(-90.0f, 0.0f, 0.0f)));
            sceneManager.updateObjectRotation(gDirLightTargetObjectNumericId, rot);
            if (auto t = findObjectById(gDirLightTargetObjectNumericId)) {
                t->setOverrideColor(glm::vec4(1.0f, 140.0f / 255.0f, 0.0f, 1.0f));
            }
            logger.Log(Logger::Level::Info, Logger::Category::Light, "Target luce direzionale aggiunto.");
        }
    }

    {
        glm::vec3 floorPos(0.0f, gFloorHeight, 0.0f);
        auto floorObj2 = sceneManager.addSystemObject("ShadowFloor", floorPos, gShadowFloorObjectNumericId, "Shadow Floor");
        if (floorObj2) {
            float extent = 50.0f;
            sceneManager.updateObjectScale(gShadowFloorObjectNumericId, glm::vec3(extent, 1.0f, extent));
            logger.Log(Logger::Level::Info, Logger::Category::Grid, "Shadow floor aggiunto.");
        }
    }

    while (!win.shouldClose()) {
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

        glm::mat4x4 lightView = glm::lookAt(gDirLight.position, gDirLight.target, glm::vec3(0.0, 1.0, 0.0));
        constexpr float orthoSize = 20.0f;
        glm::mat4x4 lightProjection = glm::orthoRH_ZO(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.01f, 100.0f);
        gShadowMapData.ViewProjection = lightProjection * lightView;

        {
            static bool isFirstFrame{ true };

            if (isFirstFrame) {
                gShadowMapData.Shader = LoadShader("Shaders/DirShadowDepth.vert", "Shaders/DirShadowDepth.frag");

                glGenTextures(1, &gShadowMapData.Image);
                glBindTexture(GL_TEXTURE_2D, gShadowMapData.Image);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
                    ShadowMapData::ShadowMapResolution, ShadowMapData::ShadowMapResolution, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                glGenFramebuffers(1, &gShadowMapData.Framebuffer);
                glBindFramebuffer(GL_FRAMEBUFFER, gShadowMapData.Framebuffer);

                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gShadowMapData.Image, 0);

                assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                isFirstFrame = false;
            }

            glUseProgram(gShadowMapData.Shader);
            glBindFramebuffer(GL_FRAMEBUFFER, gShadowMapData.Framebuffer);

            glViewport(0, 0, gShadowMapData.ShadowMapResolution, gShadowMapData.ShadowMapResolution);
            glScissor(0, 0, gShadowMapData.ShadowMapResolution, gShadowMapData.ShadowMapResolution);
            glEnable(GL_DEPTH_TEST);
            glClear(GL_DEPTH_BUFFER_BIT);

            for (auto& object : sceneManager.getObjects()) {
                unsigned int id = object->getId();

                if (id == gDirLightObjectNumericId ||
                    id == gDirLightTargetObjectNumericId ||
                    id == gShadowFloorObjectNumericId) {
                    continue;
                }

                const auto& model = object->getModel();
                const glm::mat4x4& transform = object->getModelMatrix();
                glBindVertexArray(model->getVAO());

                SetUniformMat4(gShadowMapData.Shader, "model", transform);
                SetUniformMat4(gShadowMapData.Shader, "lightSpaceMatrix", gShadowMapData.ViewProjection);

                size_t indexCount = model->getIndices().size();
                if (indexCount > 0) {
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, 0);
                }
                else if (!model->getVertices().empty()) {
                    GLsizei m_vertexCount = static_cast<GLsizei>(model->getVertices().size() / 6);
                    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
                }

                glBindVertexArray(0);
            }

            glUseProgram(0);
        }

        glEnable(GL_DEPTH_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.18f, 0.18f, 0.20f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        win.processInput();
        win.pollEvents();

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

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        float vw = ImGui::GetIO().DisplaySize.x;
        float vh = ImGui::GetIO().DisplaySize.y;
        const float SIDEBAR_LEFT_WIDTH = 260.0f;
        const float SIDEBAR_MARGIN = 6.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));

        UiWindowConfig leftSidebarCfg{
            "##LeftSidebar",
            ImVec2(SIDEBAR_MARGIN, SIDEBAR_MARGIN),
            ImVec2(SIDEBAR_LEFT_WIDTH, vh - (SIDEBAR_MARGIN * 2)),
            false,
            false,
            true,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoSavedSettings
        };

        ImGuiWindowFlags leftSidebarFlags = BeginConfiguredWindow(leftSidebarCfg);

        if (ImGui::Begin(leftSidebarCfg.id, nullptr, leftSidebarFlags)) {
            renderSectionGridMode();
            ImGui::Spacing();

            renderSectionSceneObjects();
            ImGui::Spacing();

            renderSectionCamera();
            ImGui::Spacing();

            renderSectionLighting();
            ImGui::Spacing();

            renderSectionActions();

            ImGui::End();
        }

        ImGui::PopStyleVar(2);

        auto selObjForPanel = sceneManager.getSelectedObject();
        bool lightSelected = selObjForPanel && (selObjForPanel->getId() == gDirLightObjectNumericId);
        bool lightTargetSelected = selObjForPanel && (selObjForPanel->getId() == gDirLightTargetObjectNumericId);

        if (showSelectedModelPanel && !lightSelected && !lightTargetSelected) {
            float panelWidth = std::clamp(vw * 0.20f, 220.0f, 320.0f);
            ImGui::SetNextWindowSizeConstraints(ImVec2(220, 450), ImVec2(400, FLT_MAX));
            ImGui::SetNextWindowSize(ImVec2(panelWidth, 500), ImGuiCond_Always);

            ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
            if (ImGui::Begin("Object", &showSelectedModelPanel, panelFlags)) {
                auto selectedObj = sceneManager.getSelectedObject();

                if (selectedObj) {
                    bool isLight = (selectedObj->getId() == gDirLightObjectNumericId);

                    static char nameBuffer[128] = "";
                    static bool isEditingName = false;
                    static unsigned int editingObjId = 0;

                    if (editingObjId != selectedObj->getId()) {
                        isEditingName = false;
                        editingObjId = selectedObj->getId();
                    }

                    if (!isEditingName) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
                        ImGui::TextUnformatted(selectedObj->getName().c_str());
                        ImGui::PopStyleColor();
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                            isEditingName = true;
                            strncpy(nameBuffer, selectedObj->getName().c_str(), sizeof(nameBuffer) - 1);
                        }
                    }
                    else {
                        ImGui::SetNextItemWidth(-1);
                        if (ImGui::InputText("##nameEdit", nameBuffer, sizeof(nameBuffer),
                            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                            sceneManager.renameObject(selectedObj->getId(), nameBuffer);
                            isEditingName = false;
                        }
                        if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) {
                            isEditingName = false;
                        }
                    }
                    ImGui::Spacing();

                    {
                        float btnW = 28.0f;
                        bool isT = currentGizmoOperation == ImGuizmo::TRANSLATE;
                        bool isR = currentGizmoOperation == ImGuizmo::ROTATE;
                        bool isS = currentGizmoOperation == ImGuizmo::SCALE;

                        auto modeBtn = [&](const char* label, bool active, ImGuizmo::OPERATION op) {
                            ImVec4 col = active ? ImVec4(0.45f, 0.55f, 0.70f, 1.0f) : ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
                            ImGui::PushStyleColor(ImGuiCol_Button, col);
                            if (ImGui::Button(label, ImVec2(btnW, btnW))) {
                                currentGizmoOperation = active ? ImGuizmo::UNIVERSAL : op;
                            }
                            ImGui::PopStyleColor();
                            };

                        modeBtn("T", isT, ImGuizmo::TRANSLATE);
                        ImGui::SameLine();
                        modeBtn("R", isR, ImGuizmo::ROTATE);
                        ImGui::SameLine();
                        modeBtn("S", isS, ImGuizmo::SCALE);
                        ImGui::SameLine();
                        ImGui::TextDisabled(isT ? "Move" : isR ? "Rotate" : isS ? "Scale" : "All");
                    }
                    ImGui::Spacing();

                    if (!isLight) {
                        ImGui::Separator();
                        ImGui::Text("Position");
                        
                        glm::vec3 pos = selectedObj->getPosition();
                        bool changed = false;
                        ImGui::SetNextItemWidth(-1);
                        changed = ImGui::DragFloat3("##pos", &pos.x, 0.05f, 0.0f, 0.0f, "%.2f");
                        if (changed) {
                            pos = ClampRenderablePosition(pos);
                            sceneManager.updateObjectPosition(selectedObj->getId(), pos);
                            if (isLight) gDirLight.position = pos;
                        }
                        if (ImGui::SmallButton("Reset##pos")) {
                            sceneManager.resetObjectToInitialPosition(selectedObj->getId());
                        }
                    }

                    if (!isLight) {
                        ImGui::Separator();
                        ImGui::Text("Scale");

                        glm::vec3 scale = selectedObj->getScale();
                        static bool linkScale = true;
                        ImGui::Checkbox("Link", &linkScale);
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(-1);
                        bool changed = ImGui::DragFloat3("##scale", &scale.x, 0.02f, 0.01f, 100.0f, "%.2f");
                        if (changed && linkScale) {
                            static glm::vec3 lastS = scale;
                            float ratio = 1.0f;
                            if (scale.x != lastS.x) ratio = scale.x / lastS.x;
                            else if (scale.y != lastS.y) ratio = scale.y / lastS.y;
                            else if (scale.z != lastS.z) ratio = scale.z / lastS.z;
                            scale = lastS * ratio;
                            lastS = scale;
                        }
                        if (changed) sceneManager.updateObjectScale(selectedObj->getId(), scale);
                        if (ImGui::SmallButton("Reset##scale")) {
                            sceneManager.updateObjectScale(selectedObj->getId(), glm::vec3(1.0f));
                        }
                    }

                    if (!isLight) {
                        ImGui::Separator();
                        ImGui::Text("Rotation");

                        displayedEulerAngles = glm::degrees(glm::eulerAngles(selectedObj->getRotation()));
                        ImGui::SetNextItemWidth(-1);
                        if (ImGui::DragFloat3("##rot", &displayedEulerAngles.x, 0.5f, 0.0f, 0.0f, "%.1f")) {
                            glm::quat rot = glm::quat(glm::radians(displayedEulerAngles));
                            sceneManager.updateObjectRotation(selectedObj->getId(), rot);
                        }
                        if (ImGui::SmallButton("Reset##rot")) {
                            sceneManager.updateObjectRotation(selectedObj->getId(), glm::quat(1, 0, 0, 0));
                            displayedEulerAngles = glm::vec3(0.0f);
                        }
                    }

                    if (!isLight) {
                        ImGui::Separator();
                        ImGui::Text("Appearance");

                        float btnH = 26.0f;
                        if (ImGui::Button("Texture", ImVec2(-1, btnH))) {
                            openTextureFile();
                        }
                        if (selectedObj->hasOverrideTexture()) {
                            if (ImGui::Button("Remove Texture", ImVec2(-1, btnH))) {
                                selectedObj->clearOverrideTexture();
                            }
                        }

                        if (ImGui::Button("Color", ImVec2(-1, btnH))) {
                            ImGui::OpenPopup("ColorPicker");
                        }

                        if (ImGui::BeginPopup("ColorPicker")) {
                            static glm::vec4 col = glm::vec4(1.0f);
                            if (selectedObj->hasOverrideColor()) col = selectedObj->getOverrideColor();
                            if (ImGui::ColorPicker3("##picker", &col.x, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB)) {
                                col.w = 1.0f;
                                selectedObj->setOverrideColor(col);
                            }
                            if (ImGui::Button("Clear")) {
                                selectedObj->clearOverrideColor();
                                selectedObj->clearOverrideTexture();
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }

                        // Status
                        if (selectedObj->hasOverrideTexture()) {
                            ImGui::TextColored(ImVec4(0.4f, 0.75f, 0.5f, 1.0f), "Custom texture");
                        }
                        else if (selectedObj->hasOverrideColor()) {
                            ImGui::TextColored(ImVec4(0.4f, 0.75f, 0.5f, 1.0f), "Custom color");
                        }
                        else {
                            ImGui::TextDisabled("Default");
                        }
                    }

                    ImGui::Spacing();

                    if (!isLight) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.25f, 0.25f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.30f, 0.30f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

                        if (ImGui::Button("Delete", ImVec2(-1, 28))) {
                            showDeleteConfirmation = true;
                            objectToDeleteId = selectedObj->getId();
                        }
                        ImGui::PopStyleColor(3);
                    }

                    ImGui::Checkbox("Pin", &pinSelectedModelPanel);
                }
                else {
                    ImGui::TextDisabled("Nothing selected");
                    if (!pinSelectedModelPanel) showSelectedModelPanel = false;
                }
            }
            ImGui::End();
        }

        if (shoeHelpWindow) {
            ImGui::SetNextWindowPos(ImVec2(SIDEBAR_WIDTH + 10, 10), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Shortcuts", &shoeHelpWindow, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Keyboard shortcuts");
                ImGui::Text("Camera:");
                ImGui::BulletText("Orbit: ALT + drag");
                ImGui::BulletText("Pan: CTRL + drag");
                ImGui::BulletText("Zoom: scroll");
                ImGui::BulletText("Reset: click Reset View");
                ImGui::Text("Objects:");
                ImGui::BulletText("Select: click");
                ImGui::BulletText("Move: SHIFT + drag");
                ImGui::BulletText("Move X/Y/Z: SHIFT + Q/W/E + drag");
                ImGui::Text("General:");
                ImGui::BulletText("Add: use sidebar");
                ImGui::BulletText("Delete: right-click > Delete");
                if (ImGui::Button("Close", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) shoeHelpWindow = false;
                ImGui::End();
            }
        }

        {
            UiWindowConfig fpsCfg{
                "FPS",
                ImVec2(ImGui::GetIO().DisplaySize.x - 120.0f, 8.0f),
                ImVec2(110.0f, 0.0f),
                false,
                false,
                true,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings
            };
            ImGuiWindowFlags fpsFlags = BeginConfiguredWindow(fpsCfg);
            if (ImGui::Begin(fpsCfg.id, nullptr, fpsFlags)) {
                ImGui::Text("FPS: %.1f", fps);
            }
            ImGui::End();
        }

        {
            const float pad = 8.0f;
            const ImVec2 btnSize(110, 50);
            UiWindowConfig bugCfg{
                "##BugReportBtn",
                ImVec2(ImGui::GetIO().DisplaySize.x - btnSize.x - pad,
                       ImGui::GetIO().DisplaySize.y - btnSize.y - pad),
                btnSize,
                false,
                false,
                true,
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoBackground
            };
            ImGuiWindowFlags bugFlags = BeginConfiguredWindow(bugCfg);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            if (ImGui::Begin(bugCfg.id, nullptr, bugFlags)) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.55f, 0.85f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.60f, 0.90f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.50f, 0.80f, 1.0f));

                if (ImGui::Button("Report Bug", btnSize)) {
                    OpenUrlInBrowser("https://github.com/ilmartotch/BallOfWoolProject/issues/new");
                }

                ImGui::PopStyleColor(3);

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Report a bug on GitHub");
                }
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }

        if (showFiniteSpaceInfoWindow && !gInfiniteGrid) {
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 340, 60), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(330, 0), ImGuiCond_Always);
            if (ImGui::Begin("Space Limits", &showFiniteSpaceInfoWindow,
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings)) {
                float half = gGridHalfSize + gGridBoundaryMargin;
                ImGui::TextWrapped("Finite mode active.\nArea: [-%.2f, %.2f].\nMax objects: %d", 
                    half, half, gFiniteModeMaxObjects);
                ImGui::TextWrapped("Switch to 'Infinite' mode in Grid Settings to add more.");

                if (gLastPlacementClamped || gLastMovementClamped) {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.95f, 0.4f, 0.2f, 1.0f), "%s", gBoundaryWarningMessage.c_str());
                }
                ImGui::Separator();
                if (ImGui::Button("Close", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    showFiniteSpaceInfoWindow = false;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    showFiniteSpaceInfoWindow = false;
                }
                ImGui::End();
            }
        }

        if (showDeleteConfirmation) {
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

            ImGui::OpenPopup("Delete Object?");

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(25, 20));
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));

            if (ImGui::BeginPopupModal("Delete Object?", NULL,
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings)) {

                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "CONFIRM DELETION");
                ImGui::Text("Are you sure you want to delete this object?");
                ImGui::TextDisabled("This action cannot be undone.");

                ImGui::Separator();
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

                float buttonWidth = (ImGui::GetContentRegionAvail().x -
                    ImGui::GetStyle().ItemSpacing.x * 2) / 3;

                if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
                    showDeleteConfirmation = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

                if (ImGui::Button("Delete", ImVec2(buttonWidth, 0))) {
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

                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    showDeleteConfirmation = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();
        }

        if (gShowConsole) {
            ImGui::SetNextWindowSize(ImVec2(900, 550), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints(ImVec2(600, 400), ImVec2(FLT_MAX, FLT_MAX));
            ImGui::SetNextWindowPos(
                ImVec2(ImGui::GetMainViewport()->GetCenter().x - 450,
                    ImGui::GetMainViewport()->GetCenter().y - 275),
                ImGuiCond_FirstUseEver);

            ConsoleWindow::Get().Draw(&gShowConsole);
        }

        if (errorDialog.show) {
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

            ImGui::SetNextWindowSizeConstraints(ImVec2(500, 150), ImVec2(800, 400));

            ImGui::OpenPopup(errorDialog.title.c_str());

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(25, 20));
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));

            if (ImGui::BeginPopupModal(errorDialog.title.c_str(), NULL,
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings)) {

                ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x - 10);

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

                float buttonWidthErr = 120.0f;
                float windowWidthErr = ImGui::GetContentRegionAvail().x;
                ImGui::SetCursorPosX((windowWidthErr - buttonWidthErr) * 0.5f);

                if (ImGui::Button("OK", ImVec2(buttonWidthErr, 30)) ||
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

        if (textureReplaceDialog.show) {
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

            ImGui::SetNextWindowSizeConstraints(ImVec2(600, 200), ImVec2(900, 400));

            ImGui::OpenPopup("Replace Texture?");

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(25, 20));
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.6f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.3f, 0.7f, 0.5f, 1.0f));

            if (ImGui::BeginPopupModal("Replace Texture?", NULL,
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings)) {

                ImGui::TextColored(ImVec4(0.85f, 0.70f, 0.35f, 1.0f), "HEADS UP");
                ImGui::TextWrapped("This object already has a texture.");
                ImGui::TextWrapped("Want to replace it?");

                ImGui::Separator();
                ImGui::Text("New texture:");
                ImGui::Indent(20);
                ImGui::BulletText("%s", std::filesystem::path(textureReplaceDialog.texturePath).filename().string().c_str());
                ImGui::Unindent(20);

                ImGui::Separator();
                ImGui::Text("Current:");
                ImGui::Indent(20);
                ImGui::BulletText("Existing texture");
                ImGui::Unindent(20);

                ImGui::Separator();
                ImGui::Text("Texture Usage:");
                ImGui::Indent(20);
                ImGui::BulletText("The new texture will replace the existing one.");
                ImGui::Unindent(20);

                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

                float totalWidth = ImGui::GetContentRegionAvail().x;
                float buttonWidth = (totalWidth - 10) / 3;

                if (ImGui::Button("Don't Replace", ImVec2(buttonWidth, 35))) {
                    if (textureReplaceDialog.newTextureID != 0) {
                        glDeleteTextures(1, &textureReplaceDialog.newTextureID);
                    }

                    textureReplaceDialog.show = false;
                    textureReplaceDialog.texturePath.clear();
                    textureReplaceDialog.newTextureID = 0;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.60f, 0.40f, 0.00f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.50f, 0.10f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.50f, 0.30f, 0.00f, 1.0f));

                if (ImGui::Button("Don't Ask Again", ImVec2(buttonWidth, 35))) {
                    dontAskTextureReplace = true;
                    ModelLoader::applyTextureToSelected(sceneManager, textureReplaceDialog.newTextureID);

                    textureReplaceDialog.show = false;
                    textureReplaceDialog.texturePath.clear();
                    textureReplaceDialog.newTextureID = 0;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::PopStyleColor(3);

                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.70f, 0.30f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.80f, 0.40f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.60f, 0.20f, 1.0f));

                if (ImGui::Button("Yes, Replace", ImVec2(buttonWidth, 35))) {
                    ModelLoader::applyTextureToSelected(sceneManager, textureReplaceDialog.newTextureID);

                    textureReplaceDialog.show = false;
                    textureReplaceDialog.texturePath.clear();
                    textureReplaceDialog.newTextureID = 0;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::PopStyleColor(3);

                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    if (textureReplaceDialog.newTextureID != 0) {
                        glDeleteTextures(1, &textureReplaceDialog.newTextureID);
                    }

                    textureReplaceDialog.show = false;
                    textureReplaceDialog.texturePath.clear();
                    textureReplaceDialog.newTextureID = 0;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();
        }

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

        renderScene(shader, view, projection);
        renderShadowFloor(view, projection);

        grid.render(gridShader, projection, view, mouseControl.camPos,
            gInfiniteGrid, gGridHalfSize,
            GRID_COLOR, GRID_X_AXIS_COLOR, GRID_Z_AXIS_COLOR);

        drawLightDirectionOverlay(view, projection);
        if (objectSelected) {
            renderImGuizmo(view, projection);
        }

        renderExportFormatDialog();
        renderFiniteLimitDialog();
        renderGridModeSwitchErrorDialog();

        if (successDialog.show) {
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSizeConstraints(ImVec2(500, 150), ImVec2(800, 400));

            ImGui::OpenPopup(successDialog.title.c_str());

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
                if (ImGui::Button("OK", ImVec2(buttonWidth, 30)) ||
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

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        win.swapBuffers();
    }

    modelManager.cleanup();
    pickingBuffer.cleanup();
    grid.cleanup();
    gShadows.shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteProgram(shader);
    glDeleteProgram(gridShader);
    glDeleteProgram(pickingShader);
    glDeleteProgram(dirShadowDepthShader);
    glDeleteProgram(gPlanarFloorShader);

    glfwTerminate();
    return 0;
}

// ImGuizmo
void renderImGuizmo(const glm::mat4& view, const glm::mat4& projection) {
    auto selectedObj = sceneManager.getSelectedObject();
    if (!selectedObj || !objectSelected) {
        ImGuizmo::Enable(false);
        return;
    }

    if (selectedObj->getId() == gShadowFloorObjectNumericId) {
        ImGuizmo::Enable(false);
        return;
    }

    ImGuizmo::Enable(true);
    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImGuizmo::SetRect(0, 0, static_cast<float>(gFramebufferWidth), static_cast<float>(gFramebufferHeight));

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
        glm::vec3 position, eulerDeg, scale;
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(modelMatrix),
            glm::value_ptr(position),
            glm::value_ptr(eulerDeg),
            glm::value_ptr(scale)
        );

        if (isLight) {
            position = ClampRenderablePosition(position);
            sceneManager.updateObjectPosition(selectedObj->getId(), position);
            gDirLight.position = position;
        }
        else if (isLightTarget) {
            position = ClampTargetOnFloor(position);
            sceneManager.updateObjectPosition(selectedObj->getId(), position);
            if (auto obj = sceneManager.getObjectById(gDirLightTargetObjectNumericId)) {
                obj->setPosition(position);
            }
            gDirLight.target = position;
        }
        else {
            position = ClampRenderablePosition(position);
            sceneManager.updateObjectPosition(selectedObj->getId(), position);

            displayedEulerAngles = eulerDeg;
            glm::quat rotation = glm::quat(glm::radians(displayedEulerAngles));
            sceneManager.updateObjectRotation(selectedObj->getId(), rotation);

            sceneManager.updateObjectScale(selectedObj->getId(), scale);
        }
    }

    if (auto lightObj = findObjectById(gDirLightObjectNumericId)) {
        glm::vec3 lp = ClampRenderablePosition(lightObj->getPosition());
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

// Dettagli oggetto (facoltativo, lasciato intatto)
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

// Stile
void uiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Rounding
    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    // Spacing
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 5);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;

    // Borders
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;


    style.SeparatorTextBorderSize = 0.0f;


    const ImVec4 bgDark = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    const ImVec4 bgMid = ImVec4(0.16f, 0.16f, 0.18f, 1.0f);
    const ImVec4 bgLight = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
    const ImVec4 accent = ImVec4(0.30f, 0.35f, 0.40f, 1.0f);
    const ImVec4 accentHover = ImVec4(0.35f, 0.40f, 0.45f, 1.0f);
    const ImVec4 accentActive= ImVec4(0.25f, 0.30f, 0.35f, 1.0f);    
    const ImVec4 textMain = ImVec4(0.85f, 0.85f, 0.87f, 1.0f);
    const ImVec4 textDim = ImVec4(0.55f, 0.55f, 0.58f, 1.0f);

    // Window backgrounds
    colors[ImGuiCol_WindowBg]       = bgMid;
    colors[ImGuiCol_ChildBg]        = bgDark;
    colors[ImGuiCol_PopupBg]        = bgDark;

    // Title bars
    colors[ImGuiCol_TitleBg] = bgDark;
    colors[ImGuiCol_TitleBgActive] = bgDark;
    colors[ImGuiCol_TitleBgCollapsed] = bgDark;

    // Frames
    colors[ImGuiCol_FrameBg] = bgLight;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.32f, 1.0f);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.28f, 0.32f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.33f, 0.38f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.23f, 0.27f, 1.0f);

    // Headers
    colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.32f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.32f, 0.36f, 1.0f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = bgDark;
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.34f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.38f, 0.42f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.50f, 1.0f);

    // Slider/Grab
    colors[ImGuiCol_SliderGrab]       = accent;
    colors[ImGuiCol_SliderGrabActive] = accentHover;
    colors[ImGuiCol_CheckMark]        = accent;

    // Tabs
    colors[ImGuiCol_Tab] = bgLight;
    colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.35f, 0.42f, 1.0f);
    colors[ImGuiCol_TabActive] = accent;
    colors[ImGuiCol_TabUnfocused] = bgDark;
    colors[ImGuiCol_TabUnfocusedActive] = bgLight;

    // Text
    colors[ImGuiCol_Text] = textMain;
    colors[ImGuiCol_TextDisabled] = textDim;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.35f);

    // Borders
    colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Separators
    colors[ImGuiCol_Separator] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Resize grip
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.30f, 0.30f, 0.34f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.40f, 0.45f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = accent;
}

void imGuizmoStyle() {
    ImGuizmo::Style& gizmoStyle = ImGuizmo::GetStyle();

    gizmoStyle.Colors[ImGuizmo::DIRECTION_X] = ImVec4(0.85f, 0.35f, 0.35f, 1.0f);
    gizmoStyle.Colors[ImGuizmo::DIRECTION_Y] = ImVec4(0.45f, 0.75f, 0.35f, 1.0f);
    gizmoStyle.Colors[ImGuizmo::DIRECTION_Z] = ImVec4(0.35f, 0.55f, 0.85f, 1.0f);
    gizmoStyle.Colors[ImGuizmo::PLANE_X] = ImVec4(0.85f, 0.35f, 0.35f, 0.4f);
    gizmoStyle.Colors[ImGuizmo::PLANE_Y] = ImVec4(0.45f, 0.75f, 0.35f, 0.4f);
    gizmoStyle.Colors[ImGuizmo::PLANE_Z] = ImVec4(0.35f, 0.55f, 0.85f, 0.4f);
    gizmoStyle.Colors[ImGuizmo::SELECTION] = ImVec4(0.90f, 0.75f, 0.25f, 1.0f);
    gizmoStyle.Colors[ImGuizmo::INACTIVE] = ImVec4(0.50f, 0.50f, 0.55f, 0.6f);
    gizmoStyle.Colors[ImGuizmo::TRANSLATION_LINE] = ImVec4(0.75f, 0.75f, 0.80f, 0.8f);
    gizmoStyle.Colors[ImGuizmo::SCALE_LINE] = ImVec4(0.55f, 0.55f, 0.60f, 1.0f);
    gizmoStyle.Colors[ImGuizmo::ROTATION_USING_BORDER] = ImVec4(0.90f, 0.75f, 0.25f, 1.0f);
    gizmoStyle.Colors[ImGuizmo::ROTATION_USING_FILL] = ImVec4(0.90f, 0.75f, 0.25f, 0.3f);
    gizmoStyle.Colors[ImGuizmo::HATCHED_AXIS_LINES] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
    gizmoStyle.Colors[ImGuizmo::TEXT] = ImVec4(0.85f, 0.85f, 0.87f, 1.0f);
    gizmoStyle.Colors[ImGuizmo::TEXT_SHADOW] = ImVec4(0.0f, 0.0f, 0.0f, 0.6f);
}