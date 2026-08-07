// =============================================================================
// FILE: DashboardRenderer.h
// PURPOSE: Professional full-width TUI dashboard — flicker-free, 120-column
//          layout with two-column process/sentinel view.
// --- NEW ADDITION START ---
// =============================================================================

#define _CRT_SECURE_NO_WARNINGS

#pragma once
#ifndef DASHBOARD_RENDERER_H
#define DASHBOARD_RENDERER_H

#define NOMINMAX
#include <windows.h>
#include <conio.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <ctime>
#include <algorithm>

#include "DataManager.h"
#include "ProcessingModule.h"

class DashboardRenderer {
private:
    static HANDLE hOut() { return GetStdHandle(STD_OUTPUT_HANDLE); }

    static const int W = 118;     // content width
    static const int LW = 54;     // left column (sentinel)
    static const int RW = 60;     // right column (processes)
    static const int BARW = 40;   // progress bar width

    static std::time_t& startTime() {
        static std::time_t t = std::time(nullptr);
        return t;
    }

public:
    static const WORD CLR_BODY  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    static const WORD CLR_WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    static const WORD CLR_DIM   = 8;
    static const WORD CLR_ALERT = FOREGROUND_RED | FOREGROUND_INTENSITY;
    static const WORD CLR_OK    = FOREGROUND_GREEN;
    static const WORD CLR_WARN  = FOREGROUND_RED | FOREGROUND_GREEN;

    static void setCursorPosition(short x, short y) {
        SetConsoleCursorPosition(hOut(), { x, y });
    }
    static void hideConsoleCursor() {
        CONSOLE_CURSOR_INFO ci = { 25, FALSE };
        SetConsoleCursorInfo(hOut(), &ci);
    }
    static void showConsoleCursor() {
        CONSOLE_CURSOR_INFO ci = { 25, TRUE };
        SetConsoleCursorInfo(hOut(), &ci);
    }
    static void setColor(WORD c) { SetConsoleTextAttribute(hOut(), c); }
    static void resetColor()     { setColor(CLR_BODY); }

    // ── Setup console to 120x45 ──────────────────────────────────────────────
    static void initConsole() {
        SetConsoleOutputCP(437);
        HANDLE h = hOut();
        // Shrink window first to avoid buffer-too-small error
        SMALL_RECT minR = { 0, 0, 1, 1 };
        SetConsoleWindowInfo(h, TRUE, &minR);
        COORD buf = { 122, 48 };
        SetConsoleScreenBufferSize(h, buf);
        SMALL_RECT sr = { 0, 0, 121, 47 };
        SetConsoleWindowInfo(h, TRUE, &sr);
        startTime();
    }

    // ── Progress bar [████████░░░░] XX.X% ────────────────────────────────────
    static std::string buildBar(double pct, int w = BARW) {
        if (pct < 0) pct = 0; if (pct > 100) pct = 100;
        int f = (int)(pct / 100.0 * w);
        std::string b = "[";
        for (int i = 0; i < f; ++i)    b += '\xDB';
        for (int i = f; i < w; ++i)    b += '\xB0';
        b += "]";
        std::ostringstream o;
        o << std::fixed << std::setprecision(1) << pct << "%";
        return b + " " + o.str();
    }

    // ── Sparkline ░▒▓█ ───────────────────────────────────────────────────────
    static std::string buildSparkline(const std::vector<double>& h, int n = 20) {
        std::string s;
        int st = (int)h.size() > n ? (int)h.size() - n : 0;
        for (int i = st; i < (int)h.size(); ++i) {
            if (h[i] < 25) s += '\xB0'; else if (h[i] < 50) s += '\xB1';
            else if (h[i] < 75) s += '\xB2'; else s += '\xDB';
        }
        int pad = n - (int)s.size();
        if (pad > 0) s = std::string(pad, '\xB0') + s;
        return s;
    }

    static int calcHealth(DataManager& dm) {
        int sc = 100;
        if (dm.getCPU()) { double c = dm.getCPU()->getUsagePercent();
            if (c > 90) sc -= 30; else if (c > 70) sc -= 15; else if (c > 50) sc -= 5; }
        if (dm.getRAM()) { int r = dm.getRAM()->getUsagePercent();
            if (r > 90) sc -= 30; else if (r > 70) sc -= 15; else if (r > 50) sc -= 5; }
        if (dm.getBattery() && dm.getBattery()->getHasBattery()) { int b = dm.getBattery()->getChargePercent();
            if (b < 20) sc -= 20; else if (b < 50) sc -= 10; }
        return sc < 0 ? 0 : sc;
    }

    static std::string getUptime() {
        int e = (int)difftime(std::time(nullptr), startTime());
        std::ostringstream o;
        o << std::setfill('0') << std::setw(2) << e/3600 << ":"
          << std::setw(2) << (e%3600)/60 << ":" << std::setw(2) << e%60;
        return o.str();
    }
    static std::string getTime() {
        std::time_t t = std::time(nullptr);
        char b[16]; std::strftime(b, sizeof(b), "%H:%M:%S", std::localtime(&t));
        return b;
    }

    static WORD sevColor(double p) {
        if (p >= 90) return CLR_ALERT; if (p >= 70) return CLR_WARN; return CLR_BODY;
    }

    // ── Print line padded to full width ──────────────────────────────────────
    static void pad(const std::string& s, int w = 120) {
        std::cout << s;
        int p = w - (int)s.size();
        if (p > 0) std::cout << std::string(p, ' ');
        std::cout << "\n";
    }

    static std::string hr(char c, int w = W) { return " " + std::string(w, c); }
    static std::string hrD(int w = W) { return " " + std::string(w, '\xCD'); }

    // ═══════════════════════════════════════════════════════════════════════════
    //  RENDER ZONES
    // ═══════════════════════════════════════════════════════════════════════════

    static int renderHeader(int health) {
        int row = 0;
        setCursorPosition(0, (short)row);

        std::string block(W - 4, '\xDB');
        std::string top = " \xC9" + std::string(W - 2, '\xCD') + "\xBB";
        std::string bot = " \xC8" + std::string(W - 2, '\xCD') + "\xBC";

        auto box = [](const std::string& c) {
            std::string s = " \xBA ";
            s += c;
            int p = W - 4 - (int)c.size();
            if (p > 0) s += std::string(p, ' ');
            s += " \xBA";
            return s;
        };

        setColor(CLR_DIM); pad(top); row++;
        pad(box(block)); row++;

        // Title line
        setColor(CLR_WHITE);
        std::string title = "H A R D W A R E   R E S O U R C E   &   T A S K   M O N I T O R   v 3 . 0";
        int tp = ((W - 4) - (int)title.size()) / 2;
        pad(box(std::string(tp, ' ') + title)); row++;

        // Info line: health | time | uptime
        setColor(CLR_DIM);
        std::ostringstream info;
        info << "Health: " << health << "/100"
             << "          " << getTime()
             << "          Uptime: " << getUptime();
        std::string infoStr = info.str();
        int ip = ((W - 4) - (int)infoStr.size()) / 2;
        pad(box(std::string(ip, ' ') + infoStr)); row++;

        setColor(CLR_DIM);
        pad(box(block)); row++;
        pad(bot); row++;
        resetColor();
        pad(""); row++;
        return row;
    }

    static int renderTelemetry(DataManager& dm, const std::vector<double>& cpuH, int row) {
        setCursorPosition(0, (short)row);

        setColor(CLR_DIM); pad(hrD()); row++;
        setColor(CLR_WHITE); pad("   HARDWARE TELEMETRY"); row++;
        setColor(CLR_DIM); pad(hr('-')); row++;
        resetColor();

        // CPU
        {
            double p = dm.getCPU() ? dm.getCPU()->getUsagePercent() : 0;
            std::string name = dm.getCPU() ? dm.getCPU()->getName() : "N/A";
            if (name.size() > 36) name = name.substr(0, 36);
            std::string bar = buildBar(p);
            std::string spark = buildSparkline(cpuH);

            std::cout << "   CPU   ";
            setColor(sevColor(p));
            std::cout << bar;
            resetColor();
            std::cout << "   " << name << "   ";
            setColor(CLR_DIM);
            pad("[" + spark + "]", 120 - 9 - (int)bar.size() - 3 - (int)name.size() - 3);
            resetColor();
            row++;
        }

        // RAM
        {
            double p = dm.getRAM() ? (double)dm.getRAM()->getUsagePercent() : 0;
            std::string info = "N/A";
            if (dm.getRAM()) {
                std::ostringstream o;
                o << dm.getRAM()->getUsedCapacity_MB()/1024 << " / "
                  << dm.getRAM()->getTotalCapacity_MB()/1024 << " GB";
                info = o.str();
            }
            std::string bar = buildBar(p);
            std::cout << "   RAM   ";
            setColor(sevColor(p));
            std::cout << bar;
            resetColor();
            pad("   " + info, 120 - 9 - (int)bar.size());
            row++;
        }

        // GPU
        {
            std::string name = dm.getGPU() ? dm.getGPU()->getName() : "N/A";
            if (name.size() > 50) name = name.substr(0, 50);
            std::string vram = dm.getGPU()
                ? "VRAM: " + std::to_string(dm.getGPU()->getDedicatedVRAM_MB()) + " MB" : "";
            pad("   GPU   " + name + "     " + vram);
            row++;
        }

        // Battery
        {
            if (dm.getBattery() && dm.getBattery()->getHasBattery()) {
                double p = (double)dm.getBattery()->getChargePercent();
                std::string st = dm.getBattery()->getIsCharging() ? "Charging"
                               : dm.getBattery()->getAcPluggedIn() ? "Plugged In" : "On Battery";
                std::string bar = buildBar(p);
                std::cout << "   BAT   ";
                setColor(p < 20 ? CLR_ALERT : CLR_BODY);
                std::cout << bar;
                resetColor();
                pad("   " + st, 120 - 9 - (int)bar.size());
            } else {
                pad("   BAT   No battery detected (desktop)");
            }
            row++;
        }

        setColor(CLR_DIM); pad(hrD()); row++;
        resetColor();
        pad(""); row++;
        return row;
    }

    // ── Two-column section: Sentinel (left) + Processes (right) ──────────────
    static int renderColumns(int startRow, bool breached, double threshold,
                             const std::string& topProc, double topMB,
                             const std::vector<SystemQuery::ProcessEntry>& procs) {
        int row = startRow;

        // Build left column lines (sentinel)
        std::vector<std::string> left;
        left.push_back("  PROCESS SENTINEL");
        left.push_back("  " + std::string(LW - 2, '-'));
        {
            std::ostringstream o;
            o << "  Threshold : " << std::fixed << std::setprecision(1) << threshold << "%";
            left.push_back(o.str());
        }
        left.push_back(breached ? "  Status    : !! BREACH !!" : "  Status    : NOMINAL");
        {
            std::ostringstream o;
            o << "  Offender  : " << topProc << " ("
              << std::fixed << std::setprecision(1) << topMB << " MB)";
            left.push_back(o.str());
        }
        left.push_back("");
        if (breached)
            left.push_back("  >>> RAM exceeds safe threshold! <<<");
        else
            left.push_back("  System within safe parameters.");
        left.push_back("  " + std::string(LW - 2, '-'));

        // Build right column lines (processes)
        std::vector<std::string> right;
        right.push_back("  TOP PROCESSES (by Memory)");
        right.push_back("  " + std::string(RW - 2, '-'));
        {
            std::ostringstream o;
            o << "  " << std::left << std::setw(8) << "PID"
              << std::setw(30) << "Process" << "RAM (MB)";
            right.push_back(o.str());
        }
        right.push_back("  " + std::string(RW - 2, '-'));

        int maxP = 12;
        for (int i = 0; i < maxP && i < (int)procs.size(); ++i) {
            std::ostringstream o;
            o << "  " << std::left << std::setw(8) << procs[i].pid
              << std::setw(30) << procs[i].name
              << std::fixed << std::setprecision(1) << procs[i].memoryMB;
            right.push_back(o.str());
        }
        right.push_back("  " + std::string(RW - 2, '-'));

        // Render both columns side by side
        int maxRows = (int)std::max(left.size(), right.size());
        for (int i = 0; i < maxRows; ++i) {
            setCursorPosition(0, (short)row);

            // Left column
            std::string lc = (i < (int)left.size()) ? left[i] : "";
            // Pad left column to LW + gap
            int lpad = LW + 4 - (int)lc.size();

            // Pick color for left column
            if (i == 0) setColor(CLR_WHITE);
            else if (i == 3 && breached) setColor(CLR_ALERT);
            else if (i == 6 && breached) setColor(CLR_ALERT);
            else if (i == 3 && !breached) setColor(CLR_OK);
            else if (i == 6 && !breached) setColor(CLR_DIM);
            else setColor(CLR_DIM);

            std::cout << lc;
            if (lpad > 0) std::cout << std::string(lpad, ' ');

            // Right column
            std::string rc = (i < (int)right.size()) ? right[i] : "";

            // Pick color for right column
            if (i == 0) setColor(CLR_WHITE);
            else if (i >= 4 && i < (int)right.size() - 1) {
                int pi = i - 4;
                if (pi < (int)procs.size() && procs[pi].memoryMB > 500)
                    setColor(CLR_WARN);
                else
                    resetColor();
            } else {
                setColor(CLR_DIM);
            }

            int rpad = RW - (int)rc.size();
            std::cout << rc;
            if (rpad > 0) std::cout << std::string(rpad, ' ');
            std::cout << "\n";
            resetColor();
            row++;
        }
        return row;
    }

    static int renderFooter(int startRow, int tick) {
        int row = startRow;
        setCursorPosition(0, (short)row);
        pad(""); row++;
        setColor(CLR_DIM);
        pad(hr('-'));  row++;
        {
            std::ostringstream o;
            o << "   [Q] Quit to Menu"
              << "          Tick: " << tick
              << "          Refresh: ~1s";
            pad(o.str());
            row++;
        }
        pad(hr('-')); row++;
        resetColor();
        return row;
    }

    // ── Alert Box ────────────────────────────────────────────────────────────
    static void renderAlertBox(const std::string& proc, double mb) {
        int startRow = 16;
        setCursorPosition(0, (short)startRow);

        std::string border(W, '!');
        setColor(CLR_ALERT);
        pad(""); pad(" " + border);
        pad(" !!"); pad(" !!     *** CRITICAL MEMORY THRESHOLD BREACHED ***");
        pad(" !!");
        resetColor();
        std::ostringstream o;
        o << " !!     Offender : " << proc << " (" << std::fixed
          << std::setprecision(1) << mb << " MB)";
        pad(o.str());
        setColor(CLR_ALERT);
        pad(" !!"); pad(" " + border);
        pad("");
        setColor(CLR_WHITE);
        pad("   Critical Memory Leak Detected. Terminate? (Y/N): ");
        resetColor();
    }

    static void renderKillResult(bool killed, const std::string& proc) {
        if (killed) { setColor(CLR_WHITE); pad("   [OK] Terminated: " + proc); }
        else { setColor(CLR_DIM); pad("   [--] Action dismissed. Resuming..."); }
        resetColor(); Sleep(1500);
    }

    // ── Animated wipe transition ─────────────────────────────────────────────
    static void wipeTransition(int lines = 48) {
        hideConsoleCursor();
        for (int i = 0; i < lines; ++i) {
            setCursorPosition(0, (short)i);
            setColor(CLR_DIM);
            std::cout << std::string(122, '\xB0');
            Sleep(6);
        }
        for (int i = 0; i < lines; ++i) {
            setCursorPosition(0, (short)i);
            std::cout << std::string(122, ' ');
            Sleep(6);
        }
        resetColor();
    }

    // ── OOP Showcase Screen ──────────────────────────────────────────────────
    static void renderAboutScreen() {
        system("cls");
        std::cout << "\n";
        setColor(CLR_WHITE);
        std::cout << "  +==========================================================================+\n";
        std::cout << "  |                     OOP CONCEPTS DEMONSTRATED                            |\n";
        std::cout << "  +==========================================================================+\n\n";

        setColor(CLR_WHITE); std::cout << "   [1] ABSTRACTION\n";
        setColor(CLR_DIM); std::cout << "   " << std::string(72, '-') << "\n";
        resetColor();
        std::cout << "   Abstract base classes with pure virtual methods (cannot be instantiated).\n\n";
        setColor(CLR_DIM);
        std::cout << "     HardwareComponent   ->  getUsageStats(), displayDetails(), refresh()\n";
        std::cout << "     SystemAction        ->  execute()\n\n";

        setColor(CLR_WHITE); std::cout << "   [2] INHERITANCE\n";
        setColor(CLR_DIM); std::cout << "   " << std::string(72, '-') << "\n";
        resetColor();
        std::cout << "   Derived classes inherit and extend base class behaviour.\n\n";
        setColor(CLR_DIM);
        std::cout << "     Processor, GraphicsCard, Memory, Battery  IS-A  HardwareComponent\n";
        std::cout << "     InteractiveKillAction                     IS-A  SystemAction\n\n";

        setColor(CLR_WHITE); std::cout << "   [3] POLYMORPHISM\n";
        setColor(CLR_DIM); std::cout << "   " << std::string(72, '-') << "\n";
        resetColor();
        std::cout << "   One interface, multiple runtime behaviours via vtable dispatch.\n\n";
        setColor(CLR_DIM);
        std::cout << "     ProcessingModule::update()  ->  comp->refresh()  dispatches to 4 types\n";
        std::cout << "     SystemAction* action = &killAction;  action->execute();  // vtable\n\n";

        setColor(CLR_WHITE); std::cout << "   [4] ENCAPSULATION\n";
        setColor(CLR_DIM); std::cout << "   " << std::string(72, '-') << "\n";
        resetColor();
        std::cout << "   Private data hidden behind controlled getters/setters.\n\n";
        setColor(CLR_DIM);
        std::cout << "     LoginModule       ->  private userDatabase, public authenticate()\n";
        std::cout << "     ThresholdManager  ->  private maxSafeRamLimit, validated setter\n";
        std::cout << "     DataManager       ->  private components vector, public getters\n\n";

        setColor(CLR_WHITE); std::cout << "   [5] FILE I/O & WIN32 API\n";
        setColor(CLR_DIM); std::cout << "   " << std::string(72, '-') << "\n";
        resetColor();
        std::cout << "   ReportGenerator -> std::ofstream to system_report.txt\n";
        std::cout << "   CPUID, DXGI, GlobalMemoryStatusEx, Toolhelp32, SetConsoleCursorPosition\n\n";

        setColor(CLR_DIM);
        std::cout << "   " << std::string(72, '=') << "\n";
        std::cout << "   13 headers + 1 source  |  ~1500 lines C++17  |  MSVC x64\n";
        std::cout << "   " << std::string(72, '=') << "\n\n";
        resetColor();

        std::cout << "   Press ENTER to return to menu...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
    }
};

#endif // DASHBOARD_RENDERER_H
// --- NEW ADDITION END ---
