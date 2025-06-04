#include "../Include/Window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>


Window::Window() : rotationX(0.0f), rotationY(0.0f), rotationZ(0.0f), rotationSpeed(1.0f)
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

void Window::processInput() const{
	if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(m_window, true);

	// Controlli per la rotazione
	if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
		rotationX += rotationSpeed;
	if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
		rotationX -= rotationSpeed;
	if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
		rotationY -= rotationSpeed;
	if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		rotationY += rotationSpeed;
	if (glfwGetKey(m_window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
		rotationZ += rotationSpeed;
	if (glfwGetKey(m_window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
		rotationZ -= rotationSpeed;

	// Controlli per la velocità di rotazione
	if (glfwGetKey(m_window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
		rotationSpeed += 0.1f;
	if (glfwGetKey(m_window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
		rotationSpeed = (rotationSpeed > 0.1f) ? rotationSpeed - 0.1f : 0.1f;

}