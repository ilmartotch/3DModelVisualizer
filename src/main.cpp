#include <iostream>
#include <glad/glad.h>
#include "Include/Window.h"
#include "Include/Shaders.h"
#include <filesystem>

int main() {

	//working directory
	std::cout << std::filesystem::current_path() << std::endl;

	Window win;
	if (!win.initialize(800, 600, "3D modeler")) return -1;

	//integrazione triangolo
	float vertices[] = {
		-0.5f, -0.5f, 0.0f, // sinistra-basso
		 0.5f, -0.5f, 0.0f, // destra-basso
		 0.0f,  0.5f, 0.0f  // alto
	};

	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	GLuint shader = LoadShader(
		"Shaders/Triangle.vert",
		"Shaders/Triangle.frag"
	);

	// Imposta la viewport
	/*int width, height;
	glfwGetFramebufferSize(win.m_window, &width, &height);
	glViewport(0, 0, width, height);*/
	
	//render loop
	while (!win.shouldClose()) {

		// Clear the screen
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//inout
		win.processInput();
		win.pollEvents();

		//rendering
		glUseProgram(shader);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		//REDERING 3D + UI CODE

		//check and call events + swap buffers
		win.swapBuffers();
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shader);
    
	glfwTerminate();
	return 0;
}