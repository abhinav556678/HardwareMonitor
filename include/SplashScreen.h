// =============================================================================
// FILE: SplashScreen.h
// PURPOSE: Animated startup splash screen with loading bar and system
//          detection progress. Creates a polished first impression.
// --- NEW ADDITION START ---
// =============================================================================

#pragma once
#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#define NOMINMAX
#include <windows.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

class SplashScreen {
private:
    static HANDLE hOut() { return GetStdHandle(STD_OUTPUT_HANDLE); }
    static void setPos(short x, short y) {
        SetConsoleCursorPosition(hOut(), { x, y });
    }
    static void color(WORD c) { SetConsoleTextAttribute(hOut(), c); }

    static const WORD W  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    static const WORD G  = 8;   // dark grey
    static const WORD B  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

    static void pad(const std::string& s, int w = 78) {
        std::cout << s;
        int p = w - (int)s.size();
        if (p > 0) std::cout << std::string(p, ' ');
        std::cout << "\n";
    }

    static std::string boxLine(const std::string& content, int width = 64) {
        std::string s = "  ";
        s += '\xBA';  s += ' ';
        s += content;
        int inner = width - 4;
        int p = inner - (int)content.size();
        if (p > 0) s += std::string(p, ' ');
        s += ' ';  s += '\xBA';
        return s;
    }

public:
    static void show() {
        SetConsoleOutputCP(437);
        system("cls");

        // Hide cursor
        CONSOLE_CURSOR_INFO ci = { 25, FALSE };
        SetConsoleCursorInfo(hOut(), &ci);

        short row = 1;
        std::string bar64(60, '\xCD');
        std::string block(60, '\xDB');

        // Animate: draw border line by line
        color(G);
        setPos(0, row);
        std::string top = "  \xC9" + bar64 + "\xBB";
        pad(top); row++;
        Sleep(40);

        pad(boxLine("")); row++; Sleep(20);
        pad(boxLine(block)); row++; Sleep(60);

        color(W);
        std::string t1 = "H A R D W A R E   M O N I T O R   v 3 . 0";
        int p1 = (60 - (int)t1.size()) / 2;
        pad(boxLine(std::string(p1, ' ') + t1)); row++;
        Sleep(80);

        color(G);
        std::string t2 = "Real-Time System Telemetry Dashboard";
        int p2 = (60 - (int)t2.size()) / 2;
        pad(boxLine(std::string(p2, ' ') + t2)); row++;
        Sleep(80);

        pad(boxLine(block)); row++; Sleep(60);
        pad(boxLine("")); row++; Sleep(20);

        std::string bot = "  \xC8" + bar64 + "\xBC";
        pad(bot); row++;
        Sleep(200);

        // Loading sequence
        row += 1;
        color(B);
        setPos(0, row);
        pad("    Initializing system queries...");
        row += 2;

        const char* steps[] = {
            "Scanning CPU via CPUID instruction...",
            "Querying GPU adapter via DXGI...",
            "Reading RAM via GlobalMemoryStatusEx...",
            "Checking battery via GetSystemPowerStatus...",
            "Enumerating processes via Toolhelp32...",
            "Loading sentinel modules...",
            "System ready."
        };
        int N = 7;
        int barW = 44;

        for (int s = 0; s < N; ++s) {
            // Progress bar
            setPos(4, row);
            int filled = (int)((double)(s + 1) / N * barW);
            std::string b = "[";
            for (int i = 0; i < filled; ++i) b += '\xDB';
            for (int i = filled; i < barW; ++i) b += '\xB0';
            b += "]";
            int pct = (int)((double)(s + 1) / N * 100);
            std::ostringstream oss;
            oss << b << " " << pct << "%";
            pad(oss.str());

            // Status line
            setPos(4, (short)(row + 2));
            color(s == N - 1 ? W : G);
            std::string status = "  > ";
            status += steps[s];
            pad(status);
            color(B);

            Sleep(s == N - 1 ? 400 : 250);
        }

        row += 5;
        setPos(0, row);
        color(G);
        pad("    Press any key to continue...");

        // Show cursor and wait
        ci.bVisible = TRUE;
        SetConsoleCursorInfo(hOut(), &ci);
        _getch();
        system("cls");
    }
};

#endif // SPLASH_SCREEN_H
// --- NEW ADDITION END ---
