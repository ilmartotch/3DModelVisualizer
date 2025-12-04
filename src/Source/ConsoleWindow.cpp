#include "../Include/ConsoleWindow.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <GLFW/glfw3.h>

ConsoleWindow& ConsoleWindow::Get()
{
    static ConsoleWindow s_instance;
    return s_instance;
}

ConsoleWindow::ConsoleWindow()
{
    RegisterBuiltins();
    PushConsoleLine("Console pronta. Digita 'help' per elenco comandi.");
}

void ConsoleWindow::SetTitle(const std::string& title)
{
    m_title = title;
}

void ConsoleWindow::SetMaxHistory(std::size_t maxEntries)
{
    if (maxEntries == 0) maxEntries = 1;
    m_maxHistory = maxEntries;
    while (m_consoleLines.size() > m_maxHistory)
        m_consoleLines.pop_front();
}

void ConsoleWindow::RegisterCommand(const std::string& name, const std::string& help, CommandFn fn)
{
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (m_lookup.find(lower) != m_lookup.end())
        return;
    m_commands.push_back({lower, help, std::move(fn)});
    m_lookup[lower] = m_commands.size() - 1;
}

void ConsoleWindow::PushConsoleLine(const std::string& line)
{
    m_consoleLines.emplace_back(line);
    if (m_consoleLines.size() > m_maxHistory)
        m_consoleLines.pop_front();
}

bool ConsoleWindow::PassLevelFilter(Logger::Level lvl) const
{
    switch (lvl)
    {
        case Logger::Level::Trace: return m_showTrace;
        case Logger::Level::Info:  return m_showInfo;
        case Logger::Level::Warn:  return m_showWarn;
        case Logger::Level::Error: return m_showError;
        default: return true;
    }
}

void ConsoleWindow::Tokenize(const std::string& raw, std::vector<std::string>& out) const
{
    out.clear();
    std::istringstream iss(raw);
    std::string tok;
    while (iss >> tok)
        out.push_back(tok);
}

void ConsoleWindow::ExecuteRaw(const std::string& rawLine)
{
    ExecuteCommand(rawLine);
}

void ConsoleWindow::ExecuteCommand(const std::string& raw)
{
    std::string trimmed = raw;
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char c){return !std::isspace(c);} ));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char c){return !std::isspace(c);} ).base(), trimmed.end());
    if (trimmed.empty())
        return;

    // Storico input
    if (m_inputHistory.empty() || m_inputHistory.back() != trimmed)
        m_inputHistory.push_back(trimmed);
    m_historyPos = -1;

    PushConsoleLine("> " + trimmed);

    std::vector<std::string> tokens;
    Tokenize(trimmed, tokens);
    if (tokens.empty())
        return;
    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    tokens.erase(tokens.begin());

    auto it = m_lookup.find(cmd);
    if (it == m_lookup.end())
    {
        PushConsoleLine("Comando sconosciuto: " + cmd + " (digita 'help')");
        return;
    }
    m_commands[it->second].fn(tokens);
}

void ConsoleWindow::CmdHelp(const std::vector<std::string>&)
{
    PushConsoleLine("Comandi disponibili:");
    for (const auto& c : m_commands)
        PushConsoleLine(" - " + c.name + ": " + c.help);
}

void ConsoleWindow::CmdClear(const std::vector<std::string>&)
{
    Logger::Get().Clear();
    m_consoleLines.clear();
    PushConsoleLine("[Console] Log e console svuotati");
}

void ConsoleWindow::CmdCopy(const std::vector<std::string>& args)
{
    bool onlyVisible = !args.empty() && args[0] == "visible";
    CopyVisible(onlyVisible);
    PushConsoleLine(std::string("[Console] Copiato in clipboard (") + (onlyVisible ? "visibile" : "tutto") + ")");
}

void ConsoleWindow::CmdBugReport(const std::vector<std::string>&)
{
    // Usa logger per report
    bool ok = Logger::Get().ExportAndOpenReport(true);
    PushConsoleLine(ok ? "[Console] Bug report esportato." : "[Console] Errore bug report.");
    m_lastReportTime = (float)glfwGetTime();
    m_lastReportSuccess = ok;
}

void ConsoleWindow::CmdFilter(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        m_filterText.clear();
        PushConsoleLine("[Console] Filtro testo rimosso.");
    }
    else
    {
        m_filterText = args[0];
        PushConsoleLine("[Console] Filtro impostato su: '" + m_filterText + "'");
    }
}

void ConsoleWindow::CmdPause(const std::vector<std::string>&)
{
    m_pause = !m_pause;
    PushConsoleLine(std::string("[Console] ") + (m_pause ? "PAUSA attiva: il flusso si ferma." : "PAUSA disattivata."));
}

void ConsoleWindow::CmdLevel(const std::vector<std::string>& args)
{
    if (args.size() < 2)
    {
        PushConsoleLine("Uso: level <trace|info|warn|error> <on|off>");
        return;
    }
    std::string which = args[0];
    std::string val = args[1];
    bool enable = (val == "on" || val == "1" || val == "true");

    std::transform(which.begin(), which.end(), which.begin(), ::tolower);
    if (which == "trace") m_showTrace = enable;
    else if (which == "info") m_showInfo = enable;
    else if (which == "warn") m_showWarn = enable;
    else if (which == "error") m_showError = enable;
    else {
        PushConsoleLine("Livello sconosciuto: " + which);
        return;
    }
    PushConsoleLine("[Console] Livello " + which + " -> " + (enable ? "ON" : "OFF"));
}

void ConsoleWindow::CmdSysInfo(const std::vector<std::string>&)
{
    PushConsoleLine("[Console] Info sistema (estratto):");
    PushConsoleLine(" - App Version: (Logger) gestita internamente");
    PushConsoleLine(" - GPU vendor/renderer: vedi BugReport");
    PushConsoleLine(" - Comandi principali: help, clear, copy, bugreport, filter, level, pause, history");
}

void ConsoleWindow::CmdHistory(const std::vector<std::string>&)
{
    PushConsoleLine("[Console] Cronologia input:");
    int idx = 0;
    for (const auto& line : m_inputHistory)
        PushConsoleLine("  " + std::to_string(idx++) + ": " + line);
}

void ConsoleWindow::RegisterBuiltins()
{
    RegisterCommand("help", "Elenca i comandi disponibili.", [this](auto& a){CmdHelp(a);});
    RegisterCommand("clear", "Svuota log e console.", [this](auto& a){CmdClear(a);});
    RegisterCommand("copy", "Copia log (arg opzionale: 'visible').", [this](auto& a){CmdCopy(a);});
    RegisterCommand("bugreport", "Genera markdown + apre form Notion.", [this](auto& a){CmdBugReport(a);});
    RegisterCommand("filter", "Imposta filtro testo: filter <stringa> / filter (svuota).", [this](auto& a){CmdFilter(a);});
    RegisterCommand("pause", "Toggle pausa scroll/log.", [this](auto& a){CmdPause(a);});
    RegisterCommand("level", "Abilita/disabilita livello: level <trace|info|warn|error> <on|off>.", [this](auto& a){CmdLevel(a);});
    RegisterCommand("sysinfo", "Mostra info sintetiche di sistema.", [this](auto& a){CmdSysInfo(a);});
    RegisterCommand("history", "Mostra cronologia input.", [this](auto& a){CmdHistory(a);});
}

void ConsoleWindow::CopyVisible(bool onlyVisible)
{
    std::ostringstream oss;
    oss << "=== Console Copy ===\n";
    // Riga console interne
    if (!onlyVisible)
    {
        oss << "[Console Lines]\n";
        for (const auto& l : m_consoleLines)
            oss << l << "\n";
    }
    oss << "[Log]\n";
    const auto& entries = Logger::Get().Entries();
    for (const auto& e : entries)
    {
        if (!PassLevelFilter(e.level)) continue;
        if (!m_filterText.empty() && e.text.find(m_filterText) == std::string::npos) continue;
        if (onlyVisible && m_pause) continue; // se paused e onlyVisible consideriamo freeze log
        oss << e.formattedTime << " [" << Logger::Get().LevelToString(e.level) << "] " << e.text << "\n";
    }
    Logger::Get().CopyToClipboardUtf8(oss.str());
}

void ConsoleWindow::BuildAndCopyBugReport()
{
    bool ok = Logger::Get().ExportAndOpenReport(true);
    m_lastReportSuccess = ok;
    m_lastReportTime = (float)glfwGetTime();
    PushConsoleLine(ok ? "[Console] Bug report eseguito." : "[Console] Bug report fallito.");
}

void ConsoleWindow::DrawToolbar()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.15f, 1.0f));

    // Prima riga: controlli base
    ImGui::Checkbox("Auto", &m_autoScroll);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Auto-scroll su nuove righe");

    ImGui::SameLine();
    ImGui::Checkbox("Pause", &m_pause);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Ferma il flusso di log");

    ImGui::SameLine();
    ImGui::Checkbox("TS", &m_rawTimestamp);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Mostra timestamp");

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(20, 0));
    ImGui::SameLine();

    // Livelli compatti con colori
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.65f, 0.65f, 1.0f));
    ImGui::Checkbox("Tr", &m_showTrace);
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Mostra TRACE");

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.90f, 1.00f, 1.0f));
    ImGui::Checkbox("Inf", &m_showInfo);
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Mostra INFO");

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.85f, 0.40f, 1.0f));
    ImGui::Checkbox("Wrn", &m_showWarn);
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Mostra WARN");

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.45f, 0.45f, 1.0f));
    ImGui::Checkbox("Err", &m_showError);
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Mostra ERROR");

    // Seconda riga: filtro e azioni
    ImGui::SetNextItemWidth(180.0f);
    char filterBuf[256];
    strncpy(filterBuf, m_filterText.c_str(), sizeof(filterBuf) - 1);
    filterBuf[sizeof(filterBuf) - 1] = '\0';

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.15f, 1.0f));
    if (ImGui::InputTextWithHint("##filter", " Filtra testo...", filterBuf, sizeof(filterBuf))) {
        m_filterText = filterBuf;
    }
    ImGui::PopStyleColor();

    if (!m_filterText.empty())
    {
        ImGui::SameLine();
        if (ImGui::SmallButton("X"))
            m_filterText.clear();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rimuove filtro");
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear"))
        CmdClear({});
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Svuota console e log");

    ImGui::SameLine();
    if (ImGui::Button("Copy"))
        CopyVisible(true);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Copia contenuto visibile");

    ImGui::SameLine();
    if (ImGui::Button("Report"))
        CmdBugReport({});
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Genera bug report");

    // Feedback report
    if (m_lastReportTime > 0.0f && (glfwGetTime() - m_lastReportTime) < 5.0f)
    {
        ImGui::SameLine();
        ImVec4 c = m_lastReportSuccess ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        ImGui::TextColored(c, m_lastReportSuccess ? "OK" : "ERR");
    }

    ImGui::PopStyleColor(3);
    ImGui::Separator();
}

void ConsoleWindow::DrawLogRegion()
{
    // Styling area log
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.05f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));

    ImGui::BeginChild("ConsoleRegion",
        ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2.5f),
        true,
        ImGuiWindowFlags_HorizontalScrollbar);

    // Linee console (command echo, help, ecc.) - COLORE PROMPT
    for (const auto& line : m_consoleLines)
    {
        // Evidenzia linee di comando input (che iniziano con ">")
        if (line.size() > 0 && line[0] == '>')
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.5f, 1.0f)); // Verde
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopStyleColor();
        }
        else if (line.find("[Console]") == 0)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 1.0f, 1.0f)); // Cyan
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopStyleColor();
        }
        else if (line.find("ERROR") != std::string::npos || line.find("Errore") != std::string::npos)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Rosso
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.85f, 1.0f));
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopStyleColor();
        }
    }

    // Separatore visivo tra console e log
    if (!m_consoleLines.empty() && !Logger::Get().Entries().empty())
    {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.5f, 1.0f));
        ImGui::TextUnformatted("--- Application Log ---");
        ImGui::PopStyleColor();
    }

    // Log del Logger con colori appropriati
    const auto& entries = Logger::Get().Entries();
    for (const auto& e : entries)
    {
        if (!PassLevelFilter(e.level)) continue;
        if (!m_filterText.empty() && e.text.find(m_filterText) == std::string::npos) continue;

        ImGui::PushStyleColor(ImGuiCol_Text, Logger::Get().LevelToColor(e.level));

        std::ostringstream line;
        if (m_rawTimestamp)
            line << e.formattedTime << " ";
        line << '[' << Logger::Get().LevelToString(e.level) << "] " << e.text;
        ImGui::TextUnformatted(line.str().c_str());

        ImGui::PopStyleColor();
    }

    // Auto scroll
    if (m_autoScroll && !m_pause && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void ConsoleWindow::DrawInputLine()
{
    ImGui::Separator();

    // Prompt stilizzato
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.5f, 1.0f));
    ImGui::Text(">"); // Prompt verde
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // Input field con styling
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.12f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.95f, 1.0f));

    float inputWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetNextItemWidth(inputWidth);

    if (ImGui::InputText("##console_input", m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer),
        ImGuiInputTextFlags_EnterReturnsTrue |
        ImGuiInputTextFlags_CallbackHistory |
        ImGuiInputTextFlags_CallbackCompletion,
        [](ImGuiInputTextCallbackData* data)->int {
            ConsoleWindow& self = ConsoleWindow::Get();

            // History navigation
            if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
            {
                if (data->EventKey == ImGuiKey_UpArrow)
                {
                    if (self.m_historyPos == -1)
                        self.m_historyPos = (int)self.m_inputHistory.size() - 1;
                    else if (self.m_historyPos > 0)
                        self.m_historyPos--;
                }
                else if (data->EventKey == ImGuiKey_DownArrow)
                {
                    if (self.m_historyPos != -1)
                        self.m_historyPos++;
                    if (self.m_historyPos >= (int)self.m_inputHistory.size())
                        self.m_historyPos = -1;
                }

                const char* replacement = "";
                if (self.m_historyPos >= 0 && self.m_historyPos < (int)self.m_inputHistory.size())
                    replacement = self.m_inputHistory[self.m_historyPos].c_str();

                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, replacement);
            }

            // Tab completion
            if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
            {
                std::string current(data->Buf, data->BufTextLen);
                if (!current.empty())
                {
                    // Cerca comandi che iniziano con il testo corrente
                    for (const auto& cmd : self.m_commands)
                    {
                        if (cmd.name.find(current) == 0)
                        {
                            data->DeleteChars(0, data->BufTextLen);
                            data->InsertChars(0, cmd.name.c_str());
                            break;
                        }
                    }
                }
            }

            return 0;
        }))
    {
        ExecuteCommand(m_inputBuffer);
        m_inputBuffer[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }

    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere(-1);

    ImGui::PopStyleColor(2);

    // Hint per comandi
    if (strlen(m_inputBuffer) == 0)
    {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetFrameHeight() - 2);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.5f, 0.8f));
        ImGui::TextUnformatted("Digita 'help' per la lista comandi | Tab: autocomplete | Alt+H: history");
        ImGui::PopStyleColor();
    }
}

void ConsoleWindow::Draw(bool* pOpen)
{
    if (!ImGui::Begin(m_title.c_str(), pOpen))
    {
        ImGui::End();
        return;
    }

    DrawToolbar();
    DrawLogRegion();
    DrawInputLine();

    ImGui::End();
}