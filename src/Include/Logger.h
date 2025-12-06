#pragma once

#include <deque>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <imgui.h>

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
        std::string formattedTime;
        std::string source;
    };

    // Singleton
    static Logger& Get();

    // Loggin api
    void Log(Level level, std::string_view message);
    void Log(Level level, std::string_view source, std::string_view message);
    void Logf(Level level, const char* fmt, ...);

    // Macro-friendly
    void Trace(std::string_view msg) { Log(Level::Trace, msg); }
    void Info(std::string_view msg) { Log(Level::Info, msg); }
    void Warn(std::string_view msg) { Log(Level::Warn, msg); }
    void Error(std::string_view msg) { Log(Level::Error, msg); }

    // Data access
    const std::deque<Entry>& GetEntries() const noexcept { return m_entries; }
    std::size_t GetEntryCount() const noexcept { return m_entries.size(); }

    // Entries filters
    std::vector<const Entry*> GetFilteredEntries(
        bool showTrace, bool showInfo, bool showWarn, bool showError,
        const std::string& textFilter = "") const;

	// Configuration
    void SetMaxEntries(std::size_t max);
    void SetGlfwWindow(GLFWwindow* window);
    void SetAppVersion(std::string_view version);
    void SetNotionFormUrl(const std::string& url);
    void AddExtraSystemInfo(std::string_view line);
    void Clear();

    // Export
    std::string BuildMarkdownReport() const;
    std::string BuildPlainTextLog(bool includeTimestamp = true, bool includeLevel = true) const;
    bool CopyToClipboard(const std::string& text) const;
    bool OpenUrl(const std::string& url) const;
    bool ExportAndOpenBugReport();

	// Utilities
    static const char* LevelToString(Level level) noexcept;
    static ImVec4 LevelToColor(Level level) noexcept;
    static const char* LevelToIcon(Level level) noexcept;

private:
    Logger();
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void AppendEntry(Entry&& e);
    std::string FormatTimestamp(const std::chrono::system_clock::time_point& tp) const;
    std::string BuildSystemInfo() const;
    std::string DetectOS() const noexcept;

    std::deque<Entry> m_entries;
    mutable std::mutex m_mutex;
    std::size_t m_maxEntries = 1000;

    GLFWwindow* m_glfwWindow = nullptr;
    std::string m_appVersion = "0.0.0";
    std::string m_notionFormUrl;
    std::vector<std::string> m_extraSystemInfo;
};

// Macro
#define LOG_TRACE(msg) Logger::Get().Trace(msg)
#define LOG_INFO(msg)  Logger::Get().Info(msg)
#define LOG_WARN(msg)  Logger::Get().Warn(msg)
#define LOG_ERROR(msg) Logger::Get().Error(msg)

#define LOG_TRACEF(fmt, ...) Logger::Get().Logf(Logger::Level::Trace, fmt, __VA_ARGS__)
#define LOG_INFOF(fmt, ...)  Logger::Get().Logf(Logger::Level::Info, fmt, __VA_ARGS__)
#define LOG_WARNF(fmt, ...)  Logger::Get().Logf(Logger::Level::Warn, fmt, __VA_ARGS__)
#define LOG_ERRORF(fmt, ...) Logger::Get().Logf(Logger::Level::Error, fmt, __VA_ARGS__)