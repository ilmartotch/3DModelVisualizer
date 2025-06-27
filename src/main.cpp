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

//ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

//modelli
#include "Include/ModelManager.h"
#include "../Assets/Include/CubeModel.h"
#include "../Assets/Include/SphereModel.h"
#include "../Assets/Include/PyramidModel.h"
#include "Include/Grid.h"

//variabili window
int windoWidth = 800;
int windowHeight = 600;
bool windowResized = false;
static bool shoeHelpWindow = false;
const int SIDEBAR_WIDTH = 300;
bool showModelInfo = true;
bool showSelectedModelPanel = false;

//FPS
float fps = 0.0f;
float frameTimeAccumulator = 0.0f;
int frameCount = 0;
glm::vec3 modelScale(1.0f, 1.0f, 1.0f);

bool objectSelected = false;
bool objectMoving = false;
glm::vec3 selectedObjectPosition = glm::vec3(0.0f, 0.5f, 0.0f);
glm::vec3 lastMousePos;
float moveSpeed = 0.1f;

//tracciamento RenderMode
int currentRenderMode = static_cast<int>(Model::RenderMode::SOLID);

//input comandi
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

//Crea un modelManager per la gestione dei modelli
ModelManager modelManager;

//ridimensionamento della finsestra
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	if (width > 0 && height > 0) {
		windoWidth = width;
		windowHeight = height;
		windowResized = true;

		glViewport(0, 0, width, height);
	}
}

//calcolo della nuova posizione della camera
void updateCameraPosition() {
	float radXZ = mouseControl.cameraDistance * cosf(glm::radians(mouseControl.orbitalAngleY));
	float camX = radXZ * sinf(glm::radians(mouseControl.orbitalAngleX));
	float camY = mouseControl.cameraDistance * sinf(glm::radians(mouseControl.orbitalAngleY));
	float camZ = radXZ * cosf(glm::radians(mouseControl.orbitalAngleX));

	mouseControl.camPos = glm::vec3(camX, camY + mouseControl.cameraHeight / 2, camZ);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			// Prima verifichiamo se il mouse è sopra ImGui
			if (ImGui::GetIO().WantCaptureMouse) {
				return;  // Non gestire il click se ImGui lo vuole catturare
			}

			// Controlla se ALT è premuto per modalità orbita
			if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
				glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
				mouseControl.isOrbiting = true;
				mouseControl.isPressed = false; // Disabilita la rotazione normale
			}
			else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && modelManager.hasActiveModel()) {
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

				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);
				lastMousePos = glm::vec3((float)xpos, (float)ypos, 0.0f);
			}
			else {
				if (modelManager.hasActiveModel()) {
					objectSelected = true;
				}
				mouseControl.isOrbiting = false;
				mouseControl.isPressed = true;
			}

			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
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

//callback movimento del mouse
void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
	float xoffset = (float)xpos - mouseControl.lastX;
	float yoffset = mouseControl.lastY - (float)ypos;

	mouseControl.lastX = (float)xpos;
	mouseControl.lastY = (float)ypos;

	const float sensitivity = 0.3f;

	if (objectMoving && modelManager.hasActiveModel()) {
		glm::vec3 currentMousePos = glm::vec3((float)xpos, (float)ypos, 0.0f);
		glm::vec3 mouseDelta = currentMousePos - lastMousePos;

		switch (currentMoveAxis)
		{
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
			selectedObjectPosition.y += mouseDelta.y * moveSpeed;
			selectedObjectPosition.z += mouseDelta.y * moveSpeed;
			break;
		}

		lastMousePos = currentMousePos;
	}
	else if (mouseControl.isPressed) {
		mouseControl.rotationX += yoffset * sensitivity;
		mouseControl.rotationY += xoffset * sensitivity;

		mouseControl.rotationX = fmodf(mouseControl.rotationX, 360.0f);
		mouseControl.rotationY = fmodf(mouseControl.rotationY, 360.0f);
	}

	else if (mouseControl.isOrbiting) {

		mouseControl.orbitalAngleX += xoffset * sensitivity;
		mouseControl.orbitalAngleY += yoffset * sensitivity;
		if (mouseControl.orbitalAngleY > 85.0f) mouseControl.orbitalAngleY = 85.0f;
		if (mouseControl.orbitalAngleY < -85.0f) mouseControl.orbitalAngleY = -85.0f;

		updateCameraPosition();
	}
}

//Callback per lo zoom con la rotellina del mouse
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

int main() {

	//working directory
	std::cout << std::filesystem::current_path() << std::endl;

	Window win;
	if (!win.initialize(windoWidth, windowHeight, "3D modeler")) return -1;

	//callbacks
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

	modelManager.registerModel(std::make_shared<CubeModel>());
	modelManager.registerModel(std::make_shared<SphereModel>());
	modelManager.registerModel(std::make_shared<PyramidModel>());

	// Inizializza i modelli registrati
	modelManager.initializeModels();

	//imposta il modello della griglia
	Grid grid(20, 0.5f);
	grid.initialize();

	float timeValue = 0.0f;

	std::string selectedModel = "";
	
	//render loop
	while (!win.shouldClose()) {

		//timer
		static float lastFrame = 0.0f;
		float currentFrame = static_cast<float>(glfwGetTime());
		float daltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		timeValue += daltaTime;

		//FPS
		frameTimeAccumulator += daltaTime;
		frameCount++;
		if (frameTimeAccumulator >= 0.5f) {
			fps = static_cast<float>(frameCount);
			frameTimeAccumulator = 0.0f;
			frameCount = 0;
		}

		glEnable(GL_DEPTH_TEST); // Abilita il test di profondità per la corretta visualizzazione dei modelli 3D

		// Clear the screen
		glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//input
		win.processInput();
		win.pollEvents();

		// Aggiorna le matrici una sola volta per frame
		int width, height;
		glfwGetFramebufferSize(win.getGLFWwindow(), &width, &height);
		float aspect = (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
		glm::mat4 view;

		glm::vec3 targert(0.0f, 0.0f, 0.0f);
		glm::vec3 cameraPosition;

		if (mouseControl.isOrbiting) {
			// In modalità orbitale, la posizione della telecamera viene calcolata in base agli angoli orbitali
			cameraPosition = mouseControl.camPos;
		}
		else {
			// In modalità normale, usa una posizione fissa
			cameraPosition = glm::vec3(0.0f, mouseControl.cameraHeight, mouseControl.cameraDistance);
		}

		// Usa la nuova posizione della telecamera
		view = glm::lookAt(
			mouseControl.camPos, 
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f) 
		);

		//creazione griglia
		glUseProgram(gridShader);
		glm::mat4 gridModel = glm::mat4(1.0f);
		gridModel = glm::translate(gridModel, glm::vec3(0.0f, -0.01f, 0.0f));
		SetUniformMat4(gridShader, "model", gridModel);
		SetUniformMat4(gridShader, "view", view);
		SetUniformMat4(gridShader, "projection", projection);

		grid.render(gridShader);

		//aggiornamento rendering modelli attivi
		if (modelManager.hasActiveModel()) {
			modelManager.updateActiveModel(currentFrame);

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
			if(colorLocation != -1)
				glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f);

			Model::RenderMode currentMode = static_cast<Model::RenderMode>(currentRenderMode);
			switch (currentMode) 
			{
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

			modelManager.renderActiveModel();

			if (objectSelected) {

				Model::RenderMode currentMode = static_cast<Model::RenderMode>(currentRenderMode);

				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glm::mat4 outilineModel = glm::mat4(1.0f);
				outilineModel = glm::translate(outilineModel, selectedObjectPosition);
				outilineModel = glm::rotate(outilineModel, glm::radians(mouseControl.rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
				outilineModel = glm::rotate(outilineModel, glm::radians(mouseControl.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
				outilineModel = glm::scale(outilineModel, modelScale * 1.05f);

				SetUniformMat4(shader, "model", outilineModel);

				if (colorLocation != -1)
					glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f);
				modelManager.renderActiveModel();

				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}

			if (currentMode == Model::RenderMode::WIREFRAME) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}

		void renderImGuiInterface(Window & win, float fps, std::string& selectedModel,
			glm::vec3 & selectedObjectPosition, glm::vec3 & modelScale);
		ImVec2 displaySize = ImGui::GetIO().DisplaySize;

		//generazione frame ImGui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

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
		if (ImGui::CollapsingHeader("Models", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Select a model:");
			std::vector<std::string> modelNames = modelManager.getModelNames();

			//pulsante per la selezione dei modelli
			for (const auto& name : modelNames) {
				if (ImGui::Button(name.c_str(), ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) {
					modelManager.setActiveModel(name);
					selectedModel = name;
					objectSelected = true;
					showSelectedModelPanel = true;

					//sincronizzazione stato del modello
					if (modelManager.hasActiveModel()) {
						currentRenderMode = static_cast<int>(modelManager.getActiveModel()->getRenderMode());
					}
				}
			}

			//visualizzazione solo griglia
			if (ImGui::Button("Nessun modello", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) {
				// Deseleziona il modello attivo
				modelManager.setActiveModel("");
				selectedModel = "";
				objectMoving = false;
				showSelectedModelPanel = false;
				mouseControl.rotationX = 0.0f;
				mouseControl.rotationY = 0.0f;
			}
		}

		//Modalità di rendering
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
			static const char* renderModes[] = { "Solid", "Wireframe", "Solid + Wireframe" }; 
            if (ImGui::Combo("Render Mode", &currentRenderMode, renderModes, IM_ARRAYSIZE(renderModes)))
			{
				if (modelManager.hasActiveModel()) {
					Model::RenderMode mode = static_cast<Model::RenderMode>(currentRenderMode);
					modelManager.setActiveModelRenderMode(mode);
				}
			}
		}

		//regolazione distanza telecamera
		if (ImGui::CollapsingHeader("Camera Control", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::SliderFloat("Camera Distance", &mouseControl.cameraDistance, 0.5f, 12.0f))
			{
				updateCameraPosition();
			}
		}

		//dati modello selezionato
		if (objectSelected && modelManager.hasActiveModel() && showSelectedModelPanel) {
			ImGui::Separator();
			if (ImGui::CollapsingHeader("Selected Model", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Text("Selected Model: %s", selectedModel.c_str());

				ImGui::Separator();
				ImGui::Text("Position:");

				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25f);
				ImGui::InputFloat("X##pos", &selectedObjectPosition.x, 0.1f);
				ImGui::SameLine();
				ImGui::InputFloat("Y##pos", &selectedObjectPosition.y, 0.1f);
				ImGui::SameLine();
				ImGui::InputFloat("Z##pos", &selectedObjectPosition.z, 0.1f);
				ImGui::PopItemWidth();

				ImGui::SliderFloat("Pos X", &selectedObjectPosition.x, -10.0f, 10.0f);
				ImGui::SliderFloat("Pos Y", &selectedObjectPosition.y, -10.0f, 10.0f);
				ImGui::SliderFloat("Pos Z", &selectedObjectPosition.z, -10.0f, 10.0f);

				if (ImGui::Button("Reset Position", ImVec2(ImGui::GetWindowWidth() * 0.9f, 30))) {
					selectedObjectPosition = glm::vec3(0.0f, 0.5f, 0.0f);
				}

				// Sezione dimensione
				ImGui::Separator();
				ImGui::Text("Scale");

				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25f);
				ImGui::InputFloat("X##scale", &modelScale.x, 0.1f);
				ImGui::SameLine();
				ImGui::InputFloat("Y##scale", &modelScale.y, 0.1f);
				ImGui::SameLine();
				ImGui::InputFloat("Z##scale", &modelScale.z, 0.1f);
				ImGui::PopItemWidth();

				// Checkbox per mantenere le proporzioni
				static bool maintainProportions = true;
				ImGui::Checkbox("Maintain proportions", &maintainProportions);

				// Se mantiene le proporzioni, aggiorna tutti i valori quando uno cambia
				if (maintainProportions) {
					static float lastScaleX = 1.0f;
					static float lastScaleY = 1.0f;
					static float lastScaleZ = 1.0f;

					if (modelScale.x != lastScaleX) {
						float ratio = modelScale.x / lastScaleX;
						modelScale.y *= ratio;
						modelScale.z *= ratio;
					}
					else if (modelScale.y != lastScaleY) {
						float ratio = modelScale.y / lastScaleY;
						modelScale.x *= ratio;
						modelScale.z *= ratio;
					}
					else if (modelScale.z != lastScaleZ) {
						float ratio = modelScale.z / lastScaleZ;
						modelScale.x *= ratio;
						modelScale.y *= ratio;
					}

					lastScaleX = modelScale.x;
					lastScaleY = modelScale.y;
					lastScaleZ = modelScale.z;
				}

				if (ImGui::Button("Reset Scale", ImVec2(ImGui::GetWindowWidth() * 0.9f, 30))) {
					modelScale = glm::vec3(1.0f, 1.0f, 1.0f);
				}
			}
		}

		//ridimensionamento della finestra
		if (windowResized) {
			projection = glm::perspective(glm::radians(45.0f),
				static_cast<float>(windoWidth) / static_cast<float>(windowHeight), 0.1f, 100.0f);

			ImGui::GetIO().DisplaySize = ImVec2(
				static_cast<float>(windoWidth),
				static_cast<float>(windowHeight)
			);

			windowResized = false;
		}

		bool imguiWantCaptureMouse = ImGui::GetIO().WantCaptureMouse;
		bool imguiWantCaptureKeyboard = ImGui::GetIO().WantCaptureKeyboard;

		// Controllo del mouse e della tastiera
		if (!imguiWantCaptureMouse && !imguiWantCaptureKeyboard) {
			win.processInput();
		}

		//verifica se la finestra è stata ridimensionata
		glfwGetFramebufferSize(win.getGLFWwindow(), &width, &height);
		if (width == 0 || height == 0) {
			glfwWaitEvents();
			continue;
		}

		//pulsante help
		ImGui::Separator();
		if (ImGui::Button("Help command window", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) {
			shoeHelpWindow = !shoeHelpWindow;
		}

		ImGui::End();//fine sidebar

		//finesta help
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
				if (ImGui::Button("Close", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) {
					shoeHelpWindow = false;
				}

				if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
					shoeHelpWindow = false;
				}

				ImGui::End();
			}
		}

		//FPS display
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

	// Chiudi ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glDeleteProgram(shader);
	glDeleteProgram(gridShader);

	glfwTerminate();
	return 0;
}