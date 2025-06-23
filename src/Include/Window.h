#pragma once //se il .h già esiste non ingluderlo di nuovo (evitare i duplicati)
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

class Window {
public:
    Window() : m_window(nullptr), m_width(800), m_height(600), m_resized(false) {};
    ~Window();

    // Inizializza la finestra
    bool initialize(int width, int height, const char* title);

    // Restituisce true se la finestra deve chiudersi
    bool shouldClose() const {
		return glfwWindowShouldClose(m_window);
    };

    // Poll degli eventi
    void pollEvents() const;

    // Scambia i buffer della finestra
    void swapBuffers() const;

    // Getter per il puntatore GLFWwindow*
    GLFWwindow* getGLFWwindow() const { return m_window; }

	// Funzione di callback per la gestione degli input
    void processInput() const;

	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
    bool wasResized() { bool temp = m_resized; m_resized = false; return temp; }
	void setResized() { m_resized = true; }

    //gestione rotazione
    float getRotationX() const { return rotationX; }
	float getRotationY() const { return rotationY; }
	float getRotationZ() const { return rotationZ; }

private:
    GLFWwindow* m_window;

	mutable float rotationX = 0.0f;
	mutable float rotationY = 0.0f;
    mutable float rotationZ = 0.0f;
    mutable float rotationSpeed = 1.0f;

	int m_width;
    int m_height;
    bool m_resized;
};

void Window_framebufferSizeCallback(GLFWwindow* window, int width, int height);