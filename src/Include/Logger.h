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

    struct Category {
        static constexpr const char* App = "APP";
        static constexpr const char* Window = "WINDOW";
        static constexpr const char* Render = "RENDER";
        static constexpr const char* Shader = "SHADER";
        static constexpr const char* Model = "MODEL";
        static constexpr const char* Texture = "TEXTURE";
        static constexpr const char* Scene = "SCENE";
        static constexpr const char* Export = "EXPORT";
        static constexpr const char* Input = "INPUT";
        static constexpr const char* OpenGL = "OPENGL";
        static constexpr const char* Picking = "PICKING";
        static constexpr const char* UI = "UI";
        static constexpr const char* Grid = "GRID";
        static constexpr const char* Light = "LIGHT";
        static constexpr const char* Shadow = "SHADOW";
        static constexpr const char* System = "SYSTEM";
    };

    struct Entry
    {
        Level level;
        std::chrono::system_clock::time_point timestamp;
        std::string text;
        std::string formattedTime;
        std::string category;
    };

    static Logger& Get();

    void Log(Level level, std::string_view message);
    void Log(Level level, std::string_view category, std::string_view message);
    void Logf(Level level, const char* fmt, ...);
    void Logf(Level level, const char* category, const char* fmt, ...);

    void Trace(std::string_view msg) { Log(Level::Trace, msg); }
    void Info(std::string_view msg) { Log(Level::Info, msg); }
    void Warn(std::string_view msg) { Log(Level::Warn, msg); }
    void Error(std::string_view msg) { Log(Level::Error, msg); }

    void Trace(std::string_view cat, std::string_view msg) { Log(Level::Trace, cat, msg); }
    void Info(std::string_view cat, std::string_view msg) { Log(Level::Info, cat, msg); }
    void Warn(std::string_view cat, std::string_view msg) { Log(Level::Warn, cat, msg); }
    void Error(std::string_view cat, std::string_view msg) { Log(Level::Error, cat, msg); }

    void LogModelAdded(std::string_view modelName, unsigned int id,
        size_t vertexCount, size_t indexCount, bool hasTexture);
    void LogModelLoaded(std::string_view path, size_t meshCount,
        size_t totalVertices, size_t totalIndices, float loadTimeSec);
    void LogModelLoadError(std::string_view path, std::string_view error);
    void LogModelRemoved(std::string_view modelName, unsigned int id);

    void LogTextureLoaded(std::string_view path, unsigned int glId,
        int width, int height, int channels);
    void LogTextureLoadError(std::string_view path, std::string_view error);
    void LogTextureApplied(std::string_view objectName, unsigned int objectId,
        unsigned int textureId);
    void LogTextureReplaced(std::string_view objectName, unsigned int objectId,
        unsigned int oldTextureId, unsigned int newTextureId);
    void LogTextureRemoved(std::string_view objectName, unsigned int objectId);

    void LogObjectAdded(std::string_view name, unsigned int id);
    void LogObjectRemoved(std::string_view name, unsigned int id);
    void LogObjectSelected(std::string_view name, unsigned int id);
    void LogObjectDeselected();
    void LogObjectRenamed(std::string_view oldName, std::string_view newName, unsigned int id);

    void LogGridModeChanged(bool infinite);
    void LogLightingChanged(std::string_view parameter, std::string_view value);
    void LogRenderModeChanged(std::string_view mode);

    void LogShaderLoaded(std::string_view shaderName);
    void LogShaderError(std::string_view shaderName, std::string_view error);

    void LogExportStarted(std::string_view format, std::string_view path);
    void LogExportCompleted(std::string_view path, size_t fileSize, float durationSec,
        size_t vertices, size_t faces, size_t materials);
    void LogExportError(std::string_view path, std::string_view error);

    void LogWindowResize(int logicalW, int logicalH, int fbW, int fbH);
    void LogOpenGLError(std::string_view context, unsigned int errorCode);
    void LogOpenGLInfo(std::string_view vendor, std::string_view renderer, std::string_view version);
    void LogSystemInfo(std::string_view info);

    const std::deque<Entry>& GetEntries() const noexcept { return m_entries; }
    std::size_t GetEntryCount() const noexcept { return m_entries.size(); }

    std::vector<const Entry*> GetFilteredEntries(
        bool showTrace, bool showInfo, bool showWarn, bool showError,
        const std::string& textFilter = "",
        const std::string& categoryFilter = "") const;

    void SetMaxEntries(std::size_t max);
    void SetGlfwWindow(GLFWwindow* window);
    void SetAppVersion(std::string_view version);
    void SetNotionFormUrl(const std::string& url);
    void AddExtraSystemInfo(std::string_view line);
    void Clear();

    std::string BuildMarkdownReport() const;
    std::string BuildGitHubIssueBody() const;
    std::string BuildPlainTextLog(bool includeTimestamp = true, bool includeLevel = true) const;
    bool CopyToClipboard(const std::string& text) const;
    bool OpenUrl(const std::string& url) const;
    bool ExportAndOpenBugReport();

    static const char* LevelToString(Level level) noexcept;
    static const char* LevelToIcon(Level level) noexcept;
    static ImVec4 LevelToColor(Level level) noexcept;

private:
    Logger();
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void AppendEntry(Entry&& e);
    std::string FormatTimestamp(const std::chrono::system_clock::time_point& tp) const;
    std::string FormatTimestampISO(const std::chrono::system_clock::time_point& tp) const;
    std::string BuildSystemInfo() const;
    std::string DetectOS() const noexcept;

    std::deque<Entry> m_entries;
    mutable std::mutex m_mutex;
    std::size_t m_maxEntries = 2000;

    GLFWwindow* m_glfwWindow = nullptr;
    std::string m_appVersion = "0.0.0";
    std::string m_notionFormUrl;
    std::vector<std::string> m_extraSystemInfo;

    std::string m_gpuVendor;
    std::string m_gpuRenderer;
    std::string m_gpuVersion;
};

#define LOG_TRACE(msg) Logger::Get().Trace(msg)
#define LOG_INFO(msg)  Logger::Get().Info(msg)
#define LOG_WARN(msg)  Logger::Get().Warn(msg)
#define LOG_ERROR(msg) Logger::Get().Error(msg)

#define LOG_TRACE_CAT(cat, msg) Logger::Get().Trace(cat, msg)
#define LOG_INFO_CAT(cat, msg)  Logger::Get().Info(cat, msg)
#define LOG_WARN_CAT(cat, msg)  Logger::Get().Warn(cat, msg)
#define LOG_ERROR_CAT(cat, msg) Logger::Get().Error(cat, msg)