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

//creazione sfondo
GLuint createGrid(int size, float spacing) {
	std::vector<float> gridVertices;

	// Crea linee orizzontali e verticali
	for (int i = -size; i <= size; i++) {
		// Linee orizzontali (parallele all'asse X)
		gridVertices.push_back(-size * spacing); // x1
		gridVertices.push_back(0.0f);           // y1
		gridVertices.push_back(i * spacing);    // z1

		gridVertices.push_back(size * spacing);  // x2
		gridVertices.push_back(0.0f);           // y2
		gridVertices.push_back(i * spacing);    // z2

		// Linee verticali (parallele all'asse Z)
		gridVertices.push_back(i * spacing);    // x1
		gridVertices.push_back(0.0f);           // y1
		gridVertices.push_back(-size * spacing); // z1

		gridVertices.push_back(i * spacing);    // x2
		gridVertices.push_back(0.0f);           // y2
		gridVertices.push_back(size * spacing);  // z2
	}

	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}

int main() {

	//working directory
	std::cout << std::filesystem::current_path() << std::endl;

	Window win;
	if (!win.initialize(800, 600, "3D modeler")) return -1;

	//callbacks per input
	glfwSetMouseButtonCallback(win.getGLFWwindow(), mouseButtonCallback);
	glfwSetCursorPosCallback(win.getGLFWwindow(), cursorPositionCallback);

	GLuint shader = LoadShader(
		"Shaders/Triangle.vert",
		"Shaders/Triangle.frag"
	);

	GLuint gridShader = LoadShader(
		"Shaders/Grid.vert",
		"Shaders/Grid.frag"
	);

	GLuint gridVAO = createGrid(20, 0.5f);

	//matrice di posizione
	glm::mat4 projection = glm::perspective(glm::radians(90.0f), 800.0f / 600.0f, 0.1f, 100.0f);

	//matrice camera
	glm::mat4 view = glm::lookAt(
		glm::vec3(0.0f, 1.0f, 3.0f), // Posizione camera
		glm::vec3(0.0f, 0.0f, 0.0f), // Punto di vista
		glm::vec3(0.0f, 1.0f, 0.0f)  // Vettore "up" della camera
	);

	//matrice -> shader
	SetUniformMat4(shader, "projection", projection);
	SetUniformMat4(shader, "view", view);

	float vertices[] = {

		// Posizioni				Colori
		-0.5f, 0.0f, -0.5f,          1.0f, 0.0f, 0.0f,  // 0: sinistra-back (rosso)
		 0.5f, 0.0f, -0.5f,          0.0f, 1.0f, 0.0f,  // 1: destra-back (verde)
		 0.5f, 0.0f,  0.5f,          0.0f, 0.0f, 1.0f,  // 2: destra-front (blu)
		-0.5f, 0.0f,  0.5f,          1.0f, 1.0f, 0.0f,  // 3: sinistra-front (giallo)
		// Apice
		 0.0f, 0.8f,  0.0f,          1.0f, 0.0f, 1.0f   // 4: top (magenta)
	};
	
	unsigned int indices[] = {
		0, 1, 2,  2, 3, 0,      // base
		0, 1, 4,                // lato 1
		1, 2, 4,                // lato 2
		2, 3, 4,                // lato 3
		3, 0, 4                 // lato 4
	};

	GLuint VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glEnable(GL_DEPTH_TEST);
	
	float timeValue = 0.0f;
	
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

		glBindVertexArray(gridVAO);
		glDrawArrays(GL_LINES, 0, 4 * 41);

		// Creazione del modello 3D
		glUseProgram(shader);

		//valori rotazione del mouse
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(mouseControl.rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(mouseControl.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));

		SetUniformMat4(shader, "model", model);
		SetUniformMat4(shader, "view", view);
		SetUniformMat4(shader, "projection", projection);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);

		win.swapBuffers();

		//aggiornamento colori
		if (timeValue > 0.05f) {
			timeValue = 0.0f;

			//calcolo nuovi colori
			for (int i = 0; i < 5; i++)
			{
				int colorOffset = i * 6 + 3;

				vertices[colorOffset] = 0.5f + 0.5f * sin(currentFrame + i * 0.5f);
				vertices[colorOffset + 1] = 0.5f + 0.5f * sin(currentFrame + i * 1.0f + 2.0f);
				vertices[colorOffset + 2] = 0.5f + 0.5f * sin(currentFrame + i * 1.5f + 4.0f);
			}
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		}
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteVertexArrays(1, &gridVAO);
	glDeleteProgram(shader);
	glDeleteProgram(gridShader);
    
	glfwTerminate();
	return 0;
}