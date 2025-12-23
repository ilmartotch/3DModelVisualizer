#include "../Include/Logger.h"
#include <cstdio>
#include <cstdarg>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

Logger& Logger::Get()
{
    static Logger instance;
    return instance;
}

Logger::Logger() = default;

void Logger::SetMaxEntries(std::size_t max)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxEntries = (max > 0) ? max : 1;
    while (m_entries.size() > m_maxEntries)
        m_entries.pop_front();
}

void Logger::SetGlfwWindow(GLFWwindow* window) { m_glfwWindow = window; }
void Logger::SetAppVersion(std::string_view version) { m_appVersion = std::string(version); }
void Logger::SetNotionFormUrl(const std::string& url) { m_notionFormUrl = url; }
void Logger::AddExtraSystemInfo(std::string_view line) { m_extraSystemInfo.emplace_back(line); }

void Logger::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}

std::string Logger::FormatTimestamp(const std::chrono::system_clock::time_point& tp) const
{
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S") << '.'
        << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::FormatTimestampISO(const std::chrono::system_clock::time_point& tp) const
{
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Logger::AppendEntry(Entry&& e)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.emplace_back(std::move(e));
    if (m_entries.size() > m_maxEntries)
        m_entries.pop_front();
}

void Logger::Log(Level level, std::string_view message)
{
    Entry e;
    e.level = level;
    e.timestamp = std::chrono::system_clock::now();
    e.text = std::string(message);
    e.formattedTime = FormatTimestamp(e.timestamp);
    AppendEntry(std::move(e));
}

void Logger::Log(Level level, std::string_view category, std::string_view message)
{
    Entry e;
    e.level = level;
    e.timestamp = std::chrono::system_clock::now();
    e.text = std::string(message);
    e.category = std::string(category);
    e.formattedTime = FormatTimestamp(e.timestamp);
    AppendEntry(std::move(e));
}

void Logger::Logf(Level level, const char* fmt, ...)
{
    if (!fmt) return;
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    int len = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (len > 0 && len < static_cast<int>(sizeof(buffer))) {
        Log(level, std::string_view(buffer, len));
    }
}

void Logger::Logf(Level level, const char* category, const char* fmt, ...)
{
    if (!fmt) return;
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    int len = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (len > 0 && len < static_cast<int>(sizeof(buffer))) {
        Log(level, category, std::string_view(buffer, len));
    }
}

void Logger::LogModelAdded(std::string_view modelName, unsigned int id,
    size_t vertexCount, size_t indexCount, bool hasTexture)
{
    Logf(Level::Info, Category::Model,
        "Modello aggiunto: '%.*s' (ID: %u) | Vertici: %zu | Indici: %zu | Texture: %s",
        static_cast<int>(modelName.size()), modelName.data(),
        id, vertexCount, indexCount, hasTexture ? "Si" : "No");
}

void Logger::LogModelLoaded(std::string_view path, size_t meshCount,
    size_t totalVertices, size_t totalIndices, float loadTimeSec)
{
    Logf(Level::Info, Category::Model,
        "Modello caricato: '%.*s' | Mesh: %zu | Vertici: %zu | Indici: %zu | Tempo: %.2fs",
        static_cast<int>(path.size()), path.data(),
        meshCount, totalVertices, totalIndices, loadTimeSec);
}

void Logger::LogModelLoadError(std::string_view path, std::string_view error)
{
    Logf(Level::Error, Category::Model,
        "Errore caricamento modello '%.*s': %.*s",
        static_cast<int>(path.size()), path.data(),
        static_cast<int>(error.size()), error.data());
}

void Logger::LogModelRemoved(std::string_view modelName, unsigned int id)
{
    Logf(Level::Info, Category::Model,
        "Modello rimosso: '%.*s' (ID: %u)",
        static_cast<int>(modelName.size()), modelName.data(), id);
}

void Logger::LogTextureLoaded(std::string_view path, unsigned int glId,
    int width, int height, int channels)
{
    Logf(Level::Info, Category::Texture,
        "Texture caricata: '%.*s' | GL ID: %u | %dx%d | Canali: %d",
        static_cast<int>(path.size()), path.data(),
        glId, width, height, channels);
}

void Logger::LogTextureLoadError(std::string_view path, std::string_view error)
{
    Logf(Level::Error, Category::Texture,
        "Errore caricamento texture '%.*s': %.*s",
        static_cast<int>(path.size()), path.data(),
        static_cast<int>(error.size()), error.data());
}

void Logger::LogTextureApplied(std::string_view objectName, unsigned int objectId,
    unsigned int textureId)
{
    Logf(Level::Info, Category::Texture,
        "Texture applicata a '%.*s' (ID: %u) | Texture GL ID: %u",
        static_cast<int>(objectName.size()), objectName.data(),
        objectId, textureId);
}

void Logger::LogTextureReplaced(std::string_view objectName, unsigned int objectId,
    unsigned int oldTextureId, unsigned int newTextureId)
{
    Logf(Level::Info, Category::Texture,
        "Texture sostituita su '%.*s' (ID: %u) | Vecchia: %u -> Nuova: %u",
        static_cast<int>(objectName.size()), objectName.data(),
        objectId, oldTextureId, newTextureId);
}

void Logger::LogTextureRemoved(std::string_view objectName, unsigned int objectId)
{
    Logf(Level::Info, Category::Texture,
        "Texture rimossa da '%.*s' (ID: %u)",
        static_cast<int>(objectName.size()), objectName.data(), objectId);
}

void Logger::LogObjectAdded(std::string_view name, unsigned int id)
{
    Logf(Level::Info, Category::Scene,
        "Oggetto aggiunto alla scena: '%.*s' (ID: %u)",
        static_cast<int>(name.size()), name.data(), id);
}

void Logger::LogObjectRemoved(std::string_view name, unsigned int id)
{
    Logf(Level::Info, Category::Scene,
        "Oggetto rimosso dalla scena: '%.*s' (ID: %u)",
        static_cast<int>(name.size()), name.data(), id);
}

void Logger::LogObjectSelected(std::string_view name, unsigned int id)
{
    Logf(Level::Trace, Category::Scene,
        "Oggetto selezionato: '%.*s' (ID: %u)",
        static_cast<int>(name.size()), name.data(), id);
}

void Logger::LogObjectDeselected()
{
    Log(Level::Trace, Category::Scene, "Selezione rimossa");
}

void Logger::LogObjectRenamed(std::string_view oldName, std::string_view newName, unsigned int id)
{
    Logf(Level::Info, Category::Scene,
        "Oggetto rinominato: '%.*s' -> '%.*s' (ID: %u)",
        static_cast<int>(oldName.size()), oldName.data(),
        static_cast<int>(newName.size()), newName.data(), id);
}

void Logger::LogGridModeChanged(bool infinite)
{
    Logf(Level::Info, Category::Grid,
        "Modalita' griglia cambiata: %s", infinite ? "Infinita" : "Finita");
}

void Logger::LogLightingChanged(std::string_view parameter, std::string_view value)
{
    Logf(Level::Trace, Category::Light,
        "Parametro luce modificato: %.*s = %.*s",
        static_cast<int>(parameter.size()), parameter.data(),
        static_cast<int>(value.size()), value.data());
}

void Logger::LogRenderModeChanged(std::string_view mode)
{
    Logf(Level::Info, Category::Render,
        "Modalita' rendering cambiata: %.*s",
        static_cast<int>(mode.size()), mode.data());
}

void Logger::LogShaderLoaded(std::string_view shaderName)
{
    Logf(Level::Info, Category::Shader,
        "Shader caricato: '%.*s'",
        static_cast<int>(shaderName.size()), shaderName.data());
}

void Logger::LogShaderError(std::string_view shaderName, std::string_view error)
{
    Logf(Level::Error, Category::Shader,
        "Errore shader '%.*s': %.*s",
        static_cast<int>(shaderName.size()), shaderName.data(),
        static_cast<int>(error.size()), error.data());
}

void Logger::LogExportStarted(std::string_view format, std::string_view path)
{
    Logf(Level::Info, Category::Export,
        "Export avviato | Formato: %.*s | Path: %.*s",
        static_cast<int>(format.size()), format.data(),
        static_cast<int>(path.size()), path.data());
}

void Logger::LogExportCompleted(std::string_view path, size_t fileSize, float durationSec,
    size_t vertices, size_t faces, size_t materials)
{
    Logf(Level::Info, Category::Export,
        "Export completato: '%.*s' | %zu KB | %.2fs | V:%zu F:%zu M:%zu",
        static_cast<int>(path.size()), path.data(),
        fileSize / 1024, durationSec, vertices, faces, materials);
}

void Logger::LogExportError(std::string_view path, std::string_view error)
{
    Logf(Level::Error, Category::Export,
        "Errore export '%.*s': %.*s",
        static_cast<int>(path.size()), path.data(),
        static_cast<int>(error.size()), error.data());
}

void Logger::LogWindowResize(int logicalW, int logicalH, int fbW, int fbH)
{
    Logf(Level::Trace, Category::Window,
        "Finestra ridimensionata: Logica(%dx%d) Framebuffer(%dx%d)",
        logicalW, logicalH, fbW, fbH);
}

void Logger::LogOpenGLError(std::string_view context, unsigned int errorCode)
{
    Logf(Level::Error, Category::OpenGL,
        "Errore OpenGL in '%.*s': 0x%X",
        static_cast<int>(context.size()), context.data(), errorCode);
}

void Logger::LogOpenGLInfo(std::string_view vendor, std::string_view renderer, std::string_view version)
{
    m_gpuVendor = std::string(vendor);
    m_gpuRenderer = std::string(renderer);
    m_gpuVersion = std::string(version);

    Logf(Level::Info, Category::System,
        "GPU: %.*s | %.*s | OpenGL %.*s",
        static_cast<int>(vendor.size()), vendor.data(),
        static_cast<int>(renderer.size()), renderer.data(),
        static_cast<int>(version.size()), version.data());
}

void Logger::LogSystemInfo(std::string_view info)
{
    Logf(Level::Info, Category::System, "%.*s",
        static_cast<int>(info.size()), info.data());
}

std::vector<const Logger::Entry*> Logger::GetFilteredEntries(
    bool showTrace, bool showInfo, bool showWarn, bool showError,
    const std::string& textFilter,
    const std::string& categoryFilter) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<const Entry*> result;
    result.reserve(m_entries.size());

    for (const auto& e : m_entries) {
        bool passLevel = false;
        switch (e.level) {
        case Level::Trace: passLevel = showTrace; break;
        case Level::Info:  passLevel = showInfo; break;
        case Level::Warn:  passLevel = showWarn; break;
        case Level::Error: passLevel = showError; break;
        }
        if (!passLevel) continue;

        if (!categoryFilter.empty() && e.category.find(categoryFilter) == std::string::npos)
            continue;

        if (!textFilter.empty() && e.text.find(textFilter) == std::string::npos)
            continue;

        result.push_back(&e);
    }
    return result;
}

const char* Logger::LevelToString(Level level) noexcept
{
    switch (level) {
    case Level::Trace: return "TRACE";
    case Level::Info:  return "INFO";
    case Level::Warn:  return "WARN";
    case Level::Error: return "ERROR";
    default:           return "LOG";
    }
}

const char* Logger::LevelToIcon(Level level) noexcept
{
    switch (level) {
    case Level::Trace: return "[.]";
    case Level::Info:  return "[i]";
    case Level::Warn:  return "[!]";
    case Level::Error: return "[X]";
    default:           return "[ ]";
    }
}

ImVec4 Logger::LevelToColor(Level level) noexcept
{
    switch (level) {
    case Level::Trace: return ImVec4(0.55f, 0.55f, 0.58f, 1.0f);
    case Level::Info:  return ImVec4(0.70f, 0.78f, 0.85f, 1.0f);
    case Level::Warn:  return ImVec4(0.85f, 0.70f, 0.35f, 1.0f);
    case Level::Error: return ImVec4(0.90f, 0.40f, 0.40f, 1.0f);
    default:           return ImVec4(0.85f, 0.85f, 0.87f, 1.0f);
    }
}

std::string Logger::DetectOS() const noexcept
{
#if defined(_WIN32)
    return "Windows";
#else
    return "Unknown";
#endif
}

std::string Logger::BuildSystemInfo() const
{
    std::ostringstream ss;
    ss << "## System Info\n";
    ss << "- **App Version:** " << m_appVersion << "\n";
    ss << "- **OS:** " << DetectOS() << "\n";
    if (!m_gpuRenderer.empty()) {
        ss << "- **GPU:** " << m_gpuRenderer << "\n";
        ss << "- **GPU Vendor:** " << m_gpuVendor << "\n";
        ss << "- **OpenGL Version:** " << m_gpuVersion << "\n";
    }
    auto now = std::chrono::system_clock::now();
    ss << "- **Timestamp:** " << FormatTimestampISO(now) << "\n";
    for (const auto& info : m_extraSystemInfo)
        ss << "- " << info << "\n";
    ss << "\n";
    return ss.str();
}

std::string Logger::BuildPlainTextLog(bool includeTimestamp, bool includeLevel) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ostringstream ss;
    for (const auto& e : m_entries) {
        if (includeTimestamp) ss << "[" << e.formattedTime << "] ";
        if (!e.category.empty()) ss << "[" << e.category << "] ";
        if (includeLevel) ss << "[" << LevelToString(e.level) << "] ";
        ss << e.text << "\n";
    }
    return ss.str();
}

std::string Logger::BuildMarkdownReport() const
{
    std::ostringstream md;
    md << "# Bug Report\n\n";
    md << BuildSystemInfo();
    md << "## Logs\n```\n";
    md << BuildPlainTextLog(true, true);
    md << "```\n";
    return md.str();
}

std::string Logger::BuildGitHubIssueBody() const
{
    std::ostringstream md;
    md << "## Bug Report\n\n";
    md << "<details><summary><b>System Information</b></summary>\n\n";
    md << "| Property | Value |\n|----------|-------|\n";
    md << "| App Version | `" << m_appVersion << "` |\n";
    md << "| OS | " << DetectOS() << " |\n";
    if (!m_gpuRenderer.empty()) {
        md << "| GPU | " << m_gpuRenderer << " |\n";
        md << "| OpenGL | " << m_gpuVersion << " |\n";
    }
    md << "\n</details>\n\n";
    md << "## Description\n_Descrivi il problema qui..._\n\n";
    md << "<details><summary><b>Logs</b></summary>\n\n```log\n";
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        int count = 0;
        for (auto it = m_entries.rbegin(); it != m_entries.rend() && count < 50; ++it, ++count) {
            md << "[" << it->formattedTime << "]";
            if (!it->category.empty()) md << "[" << it->category << "]";
            md << "[" << LevelToString(it->level) << "] " << it->text << "\n";
        }
    }
    md << "```\n</details>\n";
    return md.str();
}

bool Logger::CopyToClipboard(const std::string& text) const
{
    if (text.empty()) return false;
    if (m_glfwWindow) {
        glfwSetClipboardString(m_glfwWindow, text.c_str());
        return true;
    }
#ifdef _WIN32
    if (!OpenClipboard(nullptr)) return false;
    EmptyClipboard();
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hMem) {
        memcpy(GlobalLock(hMem), text.c_str(), text.size() + 1);
        GlobalUnlock(hMem);
        SetClipboardData(CF_TEXT, hMem);
    }
    CloseClipboard();
    return hMem != nullptr;
#else
    return false;
#endif
}

bool Logger::OpenUrl(const std::string& url) const
{
    if (url.empty()) return false;
#ifdef _WIN32
    return reinterpret_cast<intptr_t>(ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) > 32;
#else
    return std::system(("xdg-open \"" + url + "\"").c_str()) == 0;
#endif
}

bool Logger::ExportAndOpenBugReport()
{
    std::string report = BuildGitHubIssueBody();
    bool copied = CopyToClipboard(report);
    Log(Level::Info, Category::App, "Bug report copiato negli appunti");
    bool opened = !m_notionFormUrl.empty() ? OpenUrl(m_notionFormUrl) : true;
    return copied && opened;
}