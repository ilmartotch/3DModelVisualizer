#include <glad/glad.h>
#include "../Include/Window.h"
#include <iostream>
#include <stb_image.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

Window::Window()
    : m_window(nullptr)
    , m_width(0)
    , m_height(0)
    , m_resized(false)
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    m_width = width;
    m_height = height;

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

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

    // Load window icon
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

void Window::processInput() {
    // Reserved for future input handling
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

/*
Window class wraps GLFW window creation and management.

Key features:
- OpenGL 4.6 context creation
- Dark mode title bar on Windows via DWM API
- Window icon loading from Assets/Images/logo.png
- V-Sync enabled by default

The window resize callback is set in main.cpp via glfwSetFramebufferSizeCallback
to allow access to application-level state like picking buffer and viewport.
*/