// =============================================================================
// FILE: ReportGenerator.h
// PURPOSE: Exports a formatted system diagnostic report to a text file.
//          Demonstrates file I/O and aggregation of component data.
// --- NEW ADDITION START ---
// =============================================================================

#define _CRT_SECURE_NO_WARNINGS

#pragma once
#ifndef REPORT_GENERATOR_H
#define REPORT_GENERATOR_H

#define _CRT_SECURE_NO_WARNINGS
#include "DataManager.h"
#include "ProcessingModule.h"
#include "SystemQuery.h"

#include <fstream>
#include <string>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>

class ReportGenerator {
public:
    static bool generate(DataManager& dm, const std::string& filename = "system_report.txt") {
        std::ofstream file(filename);
        if (!file.is_open()) return false;

        // Timestamp
        std::time_t now = std::time(nullptr);
        char timeBuf[64];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        file << "================================================================\n";
        file << "  HARDWARE MONITOR — SYSTEM DIAGNOSTIC REPORT\n";
        file << "  Generated: " << timeBuf << "\n";
        file << "================================================================\n\n";

        // System
        file << "  SYSTEM\n";
        file << "  ----------------------------------------------------------------\n";
        file << "  Computer Name  : " << dm.getSystemName() << "\n";
        file << "  OS             : " << dm.getOSVersion() << "\n\n";

        // CPU
        if (dm.getCPU()) {
            dm.getCPU()->refresh();
            file << "  PROCESSOR\n";
            file << "  ----------------------------------------------------------------\n";
            file << "  Model          : " << dm.getCPU()->getName() << "\n";
            file << "  Manufacturer   : " << dm.getCPU()->getManufacturer() << "\n";
            file << "  Physical Cores : " << dm.getCPU()->getPhysicalCores() << "\n";
            file << "  Logical Procs  : " << dm.getCPU()->getLogicalProcs() << "\n";
            file << "  Rated Speed    : " << dm.getCPU()->getRatedSpeedMHz() << " MHz\n";
            file << "  Current Usage  : " << std::fixed << std::setprecision(1)
                 << dm.getCPU()->getUsagePercent() << " %\n\n";
        }

        // GPU
        if (dm.getGPU()) {
            file << "  GRAPHICS CARD\n";
            file << "  ----------------------------------------------------------------\n";
            file << "  Model          : " << dm.getGPU()->getName() << "\n";
            file << "  Manufacturer   : " << dm.getGPU()->getManufacturer() << "\n";
            file << "  Dedicated VRAM : " << dm.getGPU()->getDedicatedVRAM_MB() << " MB\n";
            file << "  Shared Memory  : " << dm.getGPU()->getSharedMemory_MB() << " MB\n\n";
        }

        // RAM
        if (dm.getRAM()) {
            dm.getRAM()->refresh();
            file << "  MEMORY (RAM)\n";
            file << "  ----------------------------------------------------------------\n";
            file << "  Total          : " << dm.getRAM()->getTotalCapacity_MB() << " MB ("
                 << dm.getRAM()->getTotalCapacity_MB() / 1024 << " GB)\n";
            file << "  Used           : " << dm.getRAM()->getUsedCapacity_MB() << " MB\n";
            file << "  Available      : " << dm.getRAM()->getAvailable_MB() << " MB\n";
            file << "  Usage          : " << dm.getRAM()->getUsagePercent() << " %\n\n";
        }

        // Battery
        if (dm.getBattery()) {
            dm.getBattery()->refresh();
            file << "  BATTERY\n";
            file << "  ----------------------------------------------------------------\n";
            if (dm.getBattery()->getHasBattery()) {
                file << "  Charge         : " << dm.getBattery()->getChargePercent() << " %\n";
                file << "  Status         : " << (dm.getBattery()->getIsCharging() ? "Charging"
                     : dm.getBattery()->getAcPluggedIn() ? "Plugged In" : "On Battery") << "\n";
            } else {
                file << "  No battery detected (desktop PC)\n";
            }
            file << "\n";
        }

        // Top Processes
        auto procs = SystemQuery::queryProcesses(10);
        file << "  TOP PROCESSES (by Memory)\n";
        file << "  ----------------------------------------------------------------\n";
        file << "  " << std::left << std::setw(8) << "PID"
             << std::setw(30) << "Name" << "RAM (MB)\n";
        file << "  ----------------------------------------------------------------\n";
        for (const auto& p : procs) {
            file << "  " << std::left << std::setw(8) << p.pid
                 << std::setw(30) << p.name
                 << std::fixed << std::setprecision(1) << p.memoryMB << "\n";
        }

        file << "\n================================================================\n";
        file << "  End of Report\n";
        file << "================================================================\n";

        file.close();
        return true;
    }

    // Interactive wrapper with UI feedback
    static void generateInteractive(DataManager& dm) {
        system("cls");
        std::cout << "\n";
        std::cout << "  +==========================================================+\n";
        std::cout << "  |              EXPORT SYSTEM DIAGNOSTIC REPORT              |\n";
        std::cout << "  +==========================================================+\n\n";

        std::string filename = "system_report.txt";
        std::cout << "  Output file: " << filename << "\n\n";
        std::cout << "  Generating report";

        // Animated dots
        for (int i = 0; i < 3; ++i) {
            Sleep(300);
            std::cout << ".";
        }
        std::cout << "\n\n";

        if (generate(dm, filename)) {
            std::cout << "  [OK] Report saved to: " << filename << "\n";
            std::cout << "       (in the same folder as the executable)\n";
        } else {
            std::cout << "  [!!] Failed to write report file.\n";
        }

        std::cout << "\n  Press ENTER to return to menu...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
    }
};

#endif // REPORT_GENERATOR_H
// --- NEW ADDITION END ---
