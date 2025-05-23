#include "../Include/Window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>


Window::Window()
{
}

Window::~Window()
{
}

bool Window::initialize(int width, int height, const char* title) {
	// Inizializza GLFW
	if (!glfwInit()) {
		return false;
	}

	// Crea la finestra
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	m_window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!m_window) {
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(m_window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		return false;
	}

	return true;

}

void Window::pollEvents() const
{
	glfwPollEvents();
}

void Window::swapBuffers() const
{
	glfwSwapBuffers(m_window);
}

GLFWwindow* Window::getGLFWwindow() const
{

	return m_window;
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}
