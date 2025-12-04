#pragma once

#include <deque>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <chrono>
#include <cstdint>
#include <imgui.h>

// Forward dichiarazione GLFW
struct GLFWwindow;

class Logger final
{
public:
    enum class Level : std::uint8_t
    {
        Trace = 0,
        Info,
        Warn,
        Error
    };

    struct Entry
    {
        Level level;
        std::chrono::system_clock::time_point timestamp;
        std::string text;
        std::string formattedTime; // Precalcolata per ridurre costo in Draw
    };

    // Singleton
    static Logger& Get();

    // Logging
    void Log(Level level, std::string_view message);
    void Logf(Level level, const char* fmt, ...);

    // Finestra ImGui
    void DrawLogWindow(bool* pOpen = nullptr);

    // Rendering custom (alternativa ad ImGui)
    void Draw(const std::function<void(const Entry&)>& renderCallback) const;

    // Bug report -> Markdown + clipboard + browser
    bool ExportAndOpenReport(bool openBrowser = true);

    // Configurazioni
    void SetMaxEntries(std::size_t maxEntries);
    void SetNotionFormUrl(const std::string& url);
    void SetGlfwWindow(GLFWwindow* window);
    void SetAppVersion(std::string_view version);
    void AddExtraSystemInfoLine(std::string_view line); // linee aggiuntive opzionali
    //void CaptureGpuInfo(); // Va chiamata dopo avere un contesto OpenGL valido
    void Clear();

    // Accesso buffer
    const std::deque<Entry>& Entries() const noexcept { return m_entries; }
    std::size_t Size() const noexcept { return m_entries.size(); }
    std::size_t Capacity() const noexcept { return m_maxEntries; }

    Logger() = default;

    static const char* LevelToString(Level level) noexcept;
    static ImVec4 LevelToColor(Level level) noexcept;

    void AppendEntry(Entry&& e);
    std::string BuildMarkdownReport() const;

    // Clipboard & Browser
    bool CopyToClipboardUtf8(const std::string& utf8) const;
    bool OpenUrlInBrowser(const char* url) const;

    // Helpers
    std::string BuildSystemInfoSection() const;
    std::string DetectOsString() const noexcept;

private:
    std::deque<Entry> m_entries;
    std::size_t m_maxEntries = 100;
    std::string m_notionFormUrl;
    GLFWwindow* m_glfwWindow = nullptr;

    // Opzioni UI
    bool m_autoScroll = true;
    bool m_showTime = true;
    bool m_showLevel = true;
    char m_filter[128] = { 0 };

    // System info / App
    std::string m_appVersion = "0.0.0";
    std::string m_gpuVendor;
    std::string m_gpuRenderer;
    std::string m_glVersion;
    std::vector<std::string> m_extraInfo;

    // Stato operazioni
    float m_lastReportTime = 0.0f; // usato per feedback
    bool m_lastReportSuccess = false;
};

// Macros di convenienza
#define LOG_TRACE(msg) Logger::Get().Log(Logger::Level::Trace, (msg))
#define LOG_INFO(msg)  Logger::Get().Log(Logger::Level::Info,  (msg))
#define LOG_WARN(msg)  Logger::Get().Log(Logger::Level::Warn,  (msg))
#define LOG_ERROR(msg) Logger::Get().Log(Logger::Level::Error, (msg))