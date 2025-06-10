#include <iostream>
#include <glad/glad.h>
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


//input comandi
struct MouseControl {
	bool isPressed = false;
	float lastX = 0.0f;
	float lastY = 0.0f;
	float rotationX = 0.0f;
	float rotationY = 0.0f;
};

MouseControl mouseControl;

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			// Prima verifichiamo se il mouse è sopra ImGui
			if (ImGui::GetIO().WantCaptureMouse) {
				return;  // Non gestire il click se ImGui lo vuole catturare
			}

			mouseControl.isPressed = true;
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			mouseControl.lastX = (float)xpos;
			mouseControl.lastY = (float)ypos;
		}
		else if (action == GLFW_RELEASE) {
			mouseControl.isPressed = false;
		}
	}
}

//callback movimento del mouse
void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
	if (mouseControl.isPressed) {
		float xoffset = (float)xpos - mouseControl.lastX;
		float yoffset = mouseControl.lastY - (float)ypos;

		mouseControl.lastX = (float)xpos;
		mouseControl.lastY = (float)ypos;

		const float sensitivity = 0.3f;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		mouseControl.rotationX += yoffset;
		mouseControl.rotationY += xoffset;

		if(mouseControl.rotationX > 89.0f)
		{
			mouseControl.rotationX = 89.0f;
		}
		if (mouseControl.rotationX < -89.0f)
		{
			mouseControl.rotationX = -89.0f;
		}
	}
}

int main() {

	//working directory
	std::cout << std::filesystem::current_path() << std::endl;

	Window win;
	if (!win.initialize(800, 600, "3D modeler")) return -1;

	//callbacks per input
	glfwSetMouseButtonCallback(win.getGLFWwindow(), mouseButtonCallback);
	glfwSetCursorPosCallback(win.getGLFWwindow(), cursorPositionCallback);

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

	//Crea un modelManager per la gestione dei modelli
	ModelManager modelManager;

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

		// Clear the screen
		glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//input
		win.processInput();
		win.pollEvents();

		// Aggiorna le matrici una sola volta per frame
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(
			glm::vec3(0.0f, 1.5f, 3.0f), // Posizione camera un po' più alta
			glm::vec3(0.0f, 0.0f, 0.0f), // Punto di vista
			glm::vec3(0.0f, 1.0f, 0.0f)  // Vettore "up" della camera
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
			model = glm::rotate(model, glm::radians(mouseControl.rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, glm::radians(mouseControl.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));

			SetUniformMat4(shader, "model", model);
			SetUniformMat4(shader, "view", view);
			SetUniformMat4(shader, "projection", projection);

			modelManager.renderActiveModel();
		}

		//generazione frame ImGui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Creazione del menu ImGui
		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(220, 0), ImGuiCond_Always);
		ImGui::Begin("Model Selector", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("Select a model:");
		std::vector<std::string> modelNames = modelManager.getModelNames();

		//pulsante per la selezione dei modelli
		for (const auto& name : modelNames) {
			if (ImGui::Button(name.c_str(), ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) {
				modelManager.setActiveModel(name);
				selectedModel = name;
			}
		}

		//visualizzazione solo griglia
		if (ImGui::Button("Nessun modello", ImVec2(ImGui::GetWindowWidth() * 0.8f, 30))) {
			// Deseleziona il modello attivo
			modelManager.setActiveModel("");
			selectedModel = "";

			mouseControl.rotationX = 0.0f;
			mouseControl.rotationY = 0.0f;
		}

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