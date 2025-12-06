#include "../Include/Logger.h"
#include <cstdio>
#include <cstdarg>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
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

Logger::Logger(){}

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

void Logger::AppendEntry(Entry&& e)
{
    // Output anche su console debug
#ifdef _DEBUG
    std::cout << "[" << LevelToString(e.level) << "] " << e.text << std::endl;
#endif

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

void Logger::Log(Level level, std::string_view source, std::string_view message)
{
    Entry e;
    e.level = level;
    e.timestamp = std::chrono::system_clock::now();
    e.text = std::string(message);
    e.source = std::string(source);
    e.formattedTime = FormatTimestamp(e.timestamp);
    AppendEntry(std::move(e));
}

void Logger::Logf(Level level, const char* fmt, ...)
{
    if (!fmt) return;

    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    int len = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len > 0 && len < static_cast<int>(sizeof(buffer))) {
        Log(level, std::string_view(buffer, len));
    }
    else if (len >= static_cast<int>(sizeof(buffer))) {
        std::vector<char> largeBuffer(len + 1);
        va_start(args, fmt);
        std::vsnprintf(largeBuffer.data(), largeBuffer.size(), fmt, args);
        va_end(args);
        Log(level, std::string_view(largeBuffer.data(), len));
    }
}

std::vector<const Logger::Entry*> Logger::GetFilteredEntries(
    bool showTrace, bool showInfo, bool showWarn, bool showError,
    const std::string& textFilter) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<const Entry*> result;
    result.reserve(m_entries.size());

    for (const auto& e : m_entries) {
        // Filtro per livello
        bool passLevel = false;
        switch (e.level) {
        case Level::Trace: passLevel = showTrace; break;
        case Level::Info:  passLevel = showInfo; break;
        case Level::Warn:  passLevel = showWarn; break;
        case Level::Error: passLevel = showError; break;
        }
        if (!passLevel) continue;

        // Filtro per testo
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

ImVec4 Logger::LevelToColor(Level level) noexcept
{
    switch (level) {
    case Level::Trace: return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    case Level::Info:  return ImVec4(0.8f, 0.9f, 1.0f, 1.0f);
    case Level::Warn:  return ImVec4(1.0f, 0.85f, 0.4f, 1.0f);
    case Level::Error: return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
    default:           return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
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

std::string Logger::DetectOS() const noexcept
{
#if defined(_WIN32)
    return "Windows";
#endif
}

std::string Logger::BuildSystemInfo() const
{
    std::ostringstream ss;
    ss << "## System Info\n";
    ss << "- App Version: " << m_appVersion << "\n";
    ss << "- OS: " << DetectOS() << "\n";

    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    ss << "- Timestamp: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";

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
        if (includeTimestamp)
            ss << "[" << e.formattedTime << "] ";
        if (includeLevel)
            ss << "[" << LevelToString(e.level) << "] ";
        ss << e.text << "\n";
    }
    return ss.str();
}

std::string Logger::BuildMarkdownReport() const
{
    std::ostringstream md;
    md << "# Bug Report\n\n";
    md << BuildSystemInfo();
    md << "## Logs\n";
    md << "```\n";
    md << BuildPlainTextLog(true, true);
    md << "```\n\n";
    md << "---\n";
    md << "Copia/incolla questo contenuto nel form e aggiungi dettagli.\n";
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
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (wideLen <= 0) return false;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, wideLen * sizeof(wchar_t));
    if (!hMem) return false;

    LPWSTR pMem = static_cast<LPWSTR>(GlobalLock(hMem));
    if (!pMem) { GlobalFree(hMem); return false; }

    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, pMem, wideLen);
    GlobalUnlock(hMem);

    if (!OpenClipboard(nullptr)) { GlobalFree(hMem); return false; }

    EmptyClipboard();
    if (!SetClipboardData(CF_UNICODETEXT, hMem)) {
        CloseClipboard();
        GlobalFree(hMem);
        return false;
    }

    CloseClipboard();
    return true;
#else
    return false;
#endif
}

bool Logger::OpenUrl(const std::string& url) const
{
    if (url.empty()) return false;
#ifdef _WIN32
    HINSTANCE res = ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<intptr_t>(res) > 32;
#else
    return std::system(("xdg-open \"" + url + "\"").c_str()) == 0;
#endif
}

bool Logger::ExportAndOpenBugReport()
{
    std::string report = BuildMarkdownReport();
    bool copied = CopyToClipboard(report);
    bool opened = !m_notionFormUrl.empty() ? OpenUrl(m_notionFormUrl) : true;
    return copied && opened;
}