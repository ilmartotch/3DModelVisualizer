#include <glad/glad.h>
#include "../Include/Window.h"
#include <iostream>
#include <stb_image.h>

#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

Window::Window()
    : m_window(nullptr)
    , m_draggingTitleBar(false)
    , m_dragStartX(0.0)
    , m_dragStartY(0.0)
    , m_windowPosX(0)
    , m_windowPosY(0)
{
}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
}

bool Window::initialize(int width, int height, const char* title) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // MODIFICA: Ripristina i bordi nativi della finestra
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Personalizza la title bar nativa (Windows 11+)
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(m_window);
    if (hwnd) {
        BOOL useDarkMode = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

        COLORREF titleBarColor = RGB(45, 45, 50);
        COLORREF titleTextColor = RGB(160, 140, 200);
        COLORREF borderColor = RGB(80, 80, 85);

        DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &titleBarColor, sizeof(titleBarColor));
        DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &titleTextColor, sizeof(titleTextColor));
        DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
    }
#endif

    // Carica icona dalla risorsa PNG
    int iconWidth, iconHeight, iconChannels;
    unsigned char* iconData = stbi_load("Assets/Images/logo.png", &iconWidth, &iconHeight, &iconChannels, 4);
    if (iconData) {
        GLFWimage icon;
        icon.width = iconWidth;
        icon.height = iconHeight;
        icon.pixels = iconData;
        glfwSetWindowIcon(m_window, 1, &icon);
        stbi_image_free(iconData);
    }

    return true;
}

bool Window::loadAppIcon(const char* iconPath) {
    return true;
}

void Window::setupBorderlessWindow() {
    // Non più necessaria per finestra con bordi nativi
}

void Window::processInput() {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        // Non chiudere con ESC
    }
}

void Window::pollEvents() const {
    glfwPollEvents();
}

void Window::swapBuffers() const {
    glfwSwapBuffers(m_window);
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

GLFWwindow* Window::getGLFWwindow() const {
    return m_window;
}