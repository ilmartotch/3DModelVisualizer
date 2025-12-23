#pragma once

#include <string>
#include <imgui.h>
#include "Logger.h"

class ConsoleWindow final
{
public:
    static ConsoleWindow& Get();

    void Draw(bool* pOpen = nullptr);

    void SetTitle(const std::string& title) { m_title = title; }

private:
    ConsoleWindow();
    ~ConsoleWindow() = default;
    ConsoleWindow(const ConsoleWindow&) = delete;
    ConsoleWindow& operator=(const ConsoleWindow&) = delete;

    void DrawToolbar();
    void DrawLogArea();
    void DrawStatusBar();

    std::string m_title = "Console";

    bool m_showTrace = true;
    bool m_showInfo = true;
    bool m_showWarn = true;
    bool m_showError = true;
    char m_filterText[256] = { 0 };

    bool m_autoScroll = true;
    bool m_showTimestamp = true;
    bool m_showLevel = true;
    bool m_wrapText = false;

    float m_lastReportTime = -10.0f;
    bool m_lastReportSuccess = false;
};