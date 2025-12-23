#pragma once
#include <GLFW/glfw3.h>
#include <string>

class Window {
public:
    Window();
    ~Window();

    bool initialize(int width, int height, const char* title);
    bool shouldClose() const;
    void pollEvents() const;
    void swapBuffers() const;
    GLFWwindow* getGLFWwindow() const;
    void processInput();

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    bool wasResized() { bool temp = m_resized; m_resized = false; return temp; }
    void setResized() { m_resized = true; }

    float getRotationX() const { return rotationX; }
    float getRotationY() const { return rotationY; }
    float getRotationZ() const { return rotationZ; }

    bool loadAppIcon(const char* iconPath);
    void setupBorderlessWindow();

    bool isDraggingTitleBar() const { return false; }
    void setDraggingTitleBar(bool dragging) {}

private:
    GLFWwindow* m_window;

    mutable float rotationX = 0.0f;
    mutable float rotationY = 0.0f;
    mutable float rotationZ = 0.0f;
    mutable float rotationSpeed = 1.0f;

    int m_width;
    int m_height;
    bool m_resized;

    bool m_draggingTitleBar;
    double m_dragStartX, m_dragStartY;
    int m_windowPosX, m_windowPosY;
};

void Window_framebufferSizeCallback(GLFWwindow* window, int width, int height);