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

private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    bool m_resized;
};