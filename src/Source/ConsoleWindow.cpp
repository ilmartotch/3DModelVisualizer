#include "../Include/ConsoleWindow.h"
#include <GLFW/glfw3.h>
#include <sstream>

ConsoleWindow& ConsoleWindow::Get()
{
    static ConsoleWindow instance;
    return instance;
}

ConsoleWindow::ConsoleWindow() = default;

void ConsoleWindow::Draw(bool* pOpen)
{
    if (pOpen && !*pOpen) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

    if (!ImGui::Begin(m_title.c_str(), pOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::Checkbox("Timestamp", &m_showTimestamp);
            ImGui::Checkbox("Level", &m_showLevel);
            ImGui::Checkbox("Auto-scroll", &m_autoScroll);
            ImGui::Checkbox("Wrap text", &m_wrapText);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Actions")) {
            if (ImGui::MenuItem("Clear Logs")) {
                Logger::Get().Clear();
            }
            if (ImGui::MenuItem("Copy All")) {
                std::string text = Logger::Get().BuildPlainTextLog(m_showTimestamp, m_showLevel);
                Logger::Get().CopyToClipboard(text);
            }
            if (ImGui::MenuItem("Bug Report")) {
                m_lastReportSuccess = Logger::Get().ExportAndOpenBugReport();
                m_lastReportTime = static_cast<float>(glfwGetTime());
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    DrawToolbar();
    DrawLogArea();
    DrawStatusBar();

    ImGui::End();
    ImGui::PopStyleVar();
}

void ConsoleWindow::DrawToolbar()
{
    // Filtri livello con colori
    ImGui::PushStyleColor(ImGuiCol_Text, Logger::LevelToColor(Logger::Level::Trace));
    ImGui::Checkbox("Trace", &m_showTrace);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, Logger::LevelToColor(Logger::Level::Info));
    ImGui::Checkbox("Info", &m_showInfo);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, Logger::LevelToColor(Logger::Level::Warn));
    ImGui::Checkbox("Warn", &m_showWarn);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, Logger::LevelToColor(Logger::Level::Error));
    ImGui::Checkbox("Error", &m_showError);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(20, 0));
    ImGui::SameLine();

    // Filtro testo
    ImGui::SetNextItemWidth(200);
    ImGui::InputTextWithHint("##filter", "Filter...", m_filterText, sizeof(m_filterText));

    if (m_filterText[0] != '\0') {
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            m_filterText[0] = '\0';
        }
    }

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(20, 0));
    ImGui::SameLine();

    // Pulsanti azione rapida
    if (ImGui::Button("Clear")) {
        Logger::Get().Clear();
    }

    ImGui::SameLine();
    if (ImGui::Button("Copy")) {
        auto entries = Logger::Get().GetFilteredEntries(
            m_showTrace, m_showInfo, m_showWarn, m_showError, m_filterText);

        std::ostringstream ss;
        for (const auto* e : entries) {
            if (m_showTimestamp) ss << "[" << e->formattedTime << "] ";
            if (m_showLevel) ss << "[" << Logger::LevelToString(e->level) << "] ";
            ss << e->text << "\n";
        }
        Logger::Get().CopyToClipboard(ss.str());
    }

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.3f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.4f, 0.3f, 1.0f));
    if (ImGui::Button("Report")) {
        m_lastReportSuccess = Logger::Get().ExportAndOpenBugReport();
        m_lastReportTime = static_cast<float>(glfwGetTime());
    }
    ImGui::PopStyleColor(2);

    // Feedback report
    float now = static_cast<float>(glfwGetTime());
    if (now - m_lastReportTime < 3.0f) {
        ImGui::SameLine();
        if (m_lastReportSuccess)
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "OK!");
        else
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Failed");
    }

    ImGui::Separator();
}

void ConsoleWindow::DrawLogArea()
{
    float footerHeight = ImGui::GetFrameHeightWithSpacing() + 8;
    float availableHeight = ImGui::GetContentRegionAvail().y - footerHeight;
    if (availableHeight < 150) availableHeight = 150;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.10f, 1.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar;
    if (m_wrapText) flags &= ~ImGuiWindowFlags_HorizontalScrollbar;

    ImGui::BeginChild("##LogScrollRegion", ImVec2(0, availableHeight), true, flags);

    auto entries = Logger::Get().GetFilteredEntries(
        m_showTrace, m_showInfo, m_showWarn, m_showError, m_filterText);

    if (entries.empty()) {
        ImGui::TextDisabled("No log entries. Logs will appear here.");
    }
    else {
        for (const auto* entry : entries) {
            ImGui::PushStyleColor(ImGuiCol_Text, Logger::LevelToColor(entry->level));

            std::ostringstream line;
            if (m_showTimestamp) line << "[" << entry->formattedTime << "] ";
            if (m_showLevel) line << Logger::LevelToIcon(entry->level) << " ";
            line << entry->text;

            if (m_wrapText)
                ImGui::TextWrapped("%s", line.str().c_str());
            else
                ImGui::TextUnformatted(line.str().c_str());

            ImGui::PopStyleColor();
        }

        if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void ConsoleWindow::DrawStatusBar()
{
    auto& logger = Logger::Get();
    size_t total = logger.GetEntryCount();
    auto filtered = logger.GetFilteredEntries(
        m_showTrace, m_showInfo, m_showWarn, m_showError, m_filterText);

    ImGui::TextDisabled("Showing %zu / %zu entries", filtered.size(), total);
    ImGui::SameLine(ImGui::GetWindowWidth() - 120);
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
}