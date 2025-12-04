#include "../Include/Logger.h"
#include <cstdio>
#include <cstdarg>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

Logger& Logger::Get()
{
    static Logger s_instance;
    return s_instance;
}

void Logger::SetMaxEntries(std::size_t maxEntries)
{
    if (maxEntries == 0) maxEntries = 1;
    m_maxEntries = maxEntries;
    while (m_entries.size() > m_maxEntries)
        m_entries.pop_front();
}

void Logger::SetNotionFormUrl(const std::string& url)
{
    m_notionFormUrl = url;
}

void Logger::SetGlfwWindow(GLFWwindow* window)
{
    m_glfwWindow = window;
}

void Logger::SetAppVersion(std::string_view version)
{
    m_appVersion.assign(version.begin(), version.end());
}

void Logger::AddExtraSystemInfoLine(std::string_view line)
{
    if (!line.empty())
        m_extraInfo.emplace_back(line);
}

//void Logger::CaptureGpuInfo()
//{
//    // Richiede contesto OpenGL valido
//    const GLubyte* vendor = glGetString(GL_VENDOR);
//    const GLubyte* renderer = glGetString(GL_RENDERER);
//    const GLubyte* version = glGetString(GL_VERSION);
//
//    m_gpuVendor = vendor ? reinterpret_cast<const char*>(vendor) : "";
//    m_gpuRenderer = renderer ? reinterpret_cast<const char*>(renderer) : "";
//    m_glVersion = version ? reinterpret_cast<const char*>(version) : "";
//}

void Logger::Clear()
{
    m_entries.clear();
}

const char* Logger::LevelToString(Level level) noexcept
{
    switch (level)
    {
    case Level::Trace: return "TRACE";
    case Level::Info:  return "INFO";
    case Level::Warn:  return "WARN";
    case Level::Error: return "ERROR";
    default:           return "LOG";
    }
}

ImVec4 Logger::LevelToColor(Level level) noexcept
{
    switch (level)
    {
    case Level::Trace: return ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
    case Level::Info:  return ImVec4(0.80f, 0.90f, 1.00f, 1.0f);
    case Level::Warn:  return ImVec4(1.00f, 0.85f, 0.40f, 1.0f);
    case Level::Error: return ImVec4(1.00f, 0.45f, 0.45f, 1.0f);
    default:           return ImVec4(1, 1, 1, 1);
    }
}

void Logger::AppendEntry(Entry&& e)
{
    m_entries.emplace_back(std::move(e));
    if (m_entries.size() > m_maxEntries)
        m_entries.pop_front();
}

void Logger::Log(Level level, std::string_view message)
{
    Entry e;
    e.level = level;
    e.timestamp = std::chrono::system_clock::now();
    e.text.assign(message.begin(), message.end());

    // Pre-format time
    auto tt = std::chrono::system_clock::to_time_t(e.timestamp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(e.timestamp.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S") << '.'
        << std::setfill('0') << std::setw(3) << ms.count();
    e.formattedTime = oss.str();

    AppendEntry(std::move(e));
}

void Logger::Logf(Level level, const char* fmt, ...)
{
    if (!fmt) return;

    char stackBuf[512];
    va_list args;
    va_start(args, fmt);
    int needed = std::vsnprintf(stackBuf, sizeof(stackBuf), fmt, args);
    va_end(args);

    if (needed >= 0 && static_cast<std::size_t>(needed) < sizeof(stackBuf))
    {
        Log(level, stackBuf);
        return;
    }

    if (needed < 0) return;

    std::vector<char> dyn(static_cast<std::size_t>(needed) + 1);
    va_start(args, fmt);
    std::vsnprintf(dyn.data(), dyn.size(), fmt, args);
    va_end(args);

    Log(level, std::string_view(dyn.data(), dyn.size() - 1));
}

void Logger::Draw(const std::function<void(const Entry&)>& renderCallback) const
{
    if (!renderCallback) return;
    for (const auto& e : m_entries)
        renderCallback(e);
}

std::string Logger::DetectOsString() const noexcept
{
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#else
    return "Unknown";
#endif
}

std::string Logger::BuildSystemInfoSection() const
{
    std::ostringstream md;
    md << "## System Info\n";
    md << "- App Version: " << m_appVersion << "\n";
    md << "- OS: " << DetectOsString() << "\n";
    md << "- GPU Vendor: " << (m_gpuVendor.empty() ? "N/A" : m_gpuVendor) << "\n";
    md << "- GPU Renderer: " << (m_gpuRenderer.empty() ? "N/A" : m_gpuRenderer) << "\n";
    md << "- OpenGL Version: " << (m_glVersion.empty() ? "N/A" : m_glVersion) << "\n";

    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    md << "- Timestamp: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";

    for (const auto& extra : m_extraInfo)
        md << "- " << extra << "\n";

    md << "\n";
    return md.str();
}

std::string Logger::BuildMarkdownReport() const
{
    std::ostringstream md;
    md << "# Bug Report\n\n";
    md << BuildSystemInfoSection();
    md << "## Logs\n";
    md << "```text\n";
    for (const auto& e : m_entries)
    {
        md << e.formattedTime
            << " [" << LevelToString(e.level) << "] "
            << e.text << "\n";
    }
    md << "```\n\n";
    md << "---\n";
    md << "Copia/incolla questo contenuto nel form Notion e aggiungi eventuali dettagli.\n";
    return md.str();
}

bool Logger::CopyToClipboardUtf8(const std::string& utf8) const
{
    if (utf8.empty()) return false;

    if (m_glfwWindow)
    {
        glfwSetClipboardString(m_glfwWindow, utf8.c_str());
        return true;
    }

#ifdef _WIN32
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wideLen <= 0) return false;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, wideLen * sizeof(wchar_t));
    if (!hMem) return false;

    LPWSTR pMem = static_cast<LPWSTR>(GlobalLock(hMem));
    if (!pMem)
    {
        GlobalFree(hMem);
        return false;
    }

    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, pMem, wideLen);
    GlobalUnlock(hMem);

    if (!OpenClipboard(nullptr))
    {
        GlobalFree(hMem);
        return false;
    }

    EmptyClipboard();
    if (!SetClipboardData(CF_UNICODETEXT, hMem))
    {
        CloseClipboard();
        GlobalFree(hMem);
        return false;
    }

    CloseClipboard();
    return true;
#else
    (void)utf8;
    return false;
#endif
}

bool Logger::OpenUrlInBrowser(const char* url) const
{
    if (!url || !*url) return false;
#ifdef _WIN32
    HINSTANCE res = ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<intptr_t>(res) > 32;
#elif __APPLE__
    std::string cmd = "open \"" + std::string(url) + "\"";
    return std::system(cmd.c_str()) == 0;
#else
    std::string cmd = "xdg-open \"" + std::string(url) + "\"";
    return std::system(cmd.c_str()) == 0;
#endif
}

bool Logger::ExportAndOpenReport(bool openBrowser)
{
    const std::string md = BuildMarkdownReport();
    bool copied = CopyToClipboardUtf8(md);

    bool opened = true;
    if (openBrowser)
    {
        opened = OpenUrlInBrowser(m_notionFormUrl.c_str());
    }

    m_lastReportTime = static_cast<float>(glfwGetTime());
    m_lastReportSuccess = copied && opened;
    return m_lastReportSuccess;
}

void Logger::DrawLogWindow(bool* pOpen)
{
    if (!ImGui::Begin("Log", pOpen, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    // Toolbar
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
    ImGui::SameLine();
    ImGui::Checkbox("Time", &m_showTime);
    ImGui::SameLine();
    ImGui::Checkbox("Level", &m_showLevel);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180.0f);
    ImGui::InputText("Filter", m_filter, IM_ARRAYSIZE(m_filter));

    // Azioni
    if (ImGui::Button("Clear"))
        Clear();
    ImGui::SameLine();
    if (ImGui::Button("Copy"))
        CopyToClipboardUtf8(BuildMarkdownReport());
    ImGui::SameLine();
    bool canReport = !m_notionFormUrl.empty();
    if (!canReport) ImGui::BeginDisabled();
    if (ImGui::Button("Bug Report"))
        ExportAndOpenReport(true);
    if (!canReport) ImGui::EndDisabled();

    // Feedback operazione bug report
    if (m_lastReportTime > 0.0f && (glfwGetTime() - m_lastReportTime) < 5.0f)
    {
        ImGui::SameLine();
        if (m_lastReportSuccess)
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Report esportato!");
        else
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Report fallito");
    }

    ImGui::Separator();

    ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& e : m_entries)
    {
        if (m_filter[0] != '\0')
        {
            std::string_view sv = e.text;
            if (sv.find(m_filter) == std::string_view::npos)
                continue;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, LevelToColor(e.level));
        std::ostringstream line;
        if (m_showTime)
        {
            line << '[' << e.formattedTime << "] ";
        }
        if (m_showLevel)
        {
            line << '[' << LevelToString(e.level) << "] ";
        }
        line << e.text;
        ImGui::TextUnformatted(line.str().c_str());
        ImGui::PopStyleColor();
    }

    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}