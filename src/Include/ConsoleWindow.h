#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <deque>
#include <cstdint>
#include <imgui.h>
#include "Logger.h"

class ConsoleWindow final
{
public:
    using CommandFn = std::function<void(const std::vector<std::string>& args)>;

    static ConsoleWindow& Get();

    // Disegna la finestra 
    void Draw(bool* pOpen = nullptr);

    // Registrazione comandi (nome in minuscolo)
    void RegisterCommand(const std::string& name, const std::string& help, CommandFn fn);

    // Inserimento di una riga di console interna (non va nel Logger)
    void PushConsoleLine(const std::string& line);

    // Configurazione
    void SetMaxHistory(std::size_t maxEntries);
    void SetTitle(const std::string& title);

    // Esecuzione manuale di comando
    void ExecuteRaw(const std::string& rawLine);


private:
    ConsoleWindow();

    // Helpers
    void DrawToolbar();
    void DrawLogRegion();
    void DrawInputLine();
    void ExecuteCommand(const std::string& raw);
    void Tokenize(const std::string& raw, std::vector<std::string>& out) const;
    bool PassLevelFilter(Logger::Level lvl) const;
    void CopyVisible(bool onlyVisible);
    void BuildAndCopyBugReport();

    // Built-in commands
    void RegisterBuiltins();
    void CmdHelp(const std::vector<std::string>& args);
    void CmdClear(const std::vector<std::string>& args);
    void CmdCopy(const std::vector<std::string>& args);
    void CmdBugReport(const std::vector<std::string>& args);
    void CmdFilter(const std::vector<std::string>& args);
    void CmdPause(const std::vector<std::string>& args);
    void CmdLevel(const std::vector<std::string>& args);
    void CmdSysInfo(const std::vector<std::string>& args);
    void CmdHistory(const std::vector<std::string>& args);

    struct CommandInfo {
        std::string name;
        std::string help;
        CommandFn fn;
    };

    std::string m_title = "Console";
    char m_inputBuffer[512] = {0};
    std::string m_filterText;
    bool m_showTrace = true;
    bool m_showInfo = true;
    bool m_showWarn = true;
    bool m_showError = true;
    bool m_pause = false;
    bool m_autoScroll = true;
    bool m_rawTimestamp = false;

    std::vector<CommandInfo> m_commands;
    std::unordered_map<std::string, std::size_t> m_lookup; // name -> index
    std::deque<std::string> m_consoleLines;
    std::size_t m_maxHistory = 500;

    // Input history
    std::vector<std::string> m_inputHistory;
    int m_historyPos = -1;

    // Stato feedback bug report
    float m_lastReportTime = 0.0f;
    bool m_lastReportSuccess = false;
};