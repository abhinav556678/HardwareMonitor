// =============================================================================
// FILE: ProcessingModule.h
// PURPOSE: Processing Module — refreshes REAL system data each tick, shows
//          real running processes, and computes live statistics.
//          Demonstrates POLYMORPHISM — calls refresh() on each component
//          through base-class pointers (runtime virtual dispatch).
// =============================================================================

#pragma once
#ifndef PROCESSING_MODULE_H
#define PROCESSING_MODULE_H

#include "DataManager.h"
#include "SystemQuery.h"
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <iomanip>
#include <sstream>

class ProcessingModule {
    // -- [ENCAPSULATION] Private State -----------------------------------------
private:
    DataManager& dataManager;
    int simulationTick;

    // Rolling history (last 10 ticks)
    std::vector<double> cpuHistory;

    // Cached process list
    std::vector<SystemQuery::ProcessEntry> processList;

public:
    explicit ProcessingModule(DataManager& dm)
        : dataManager(dm), simulationTick(0) {}

    // -- [POLYMORPHISM] update() -----------------------------------------------
    // Calls refresh() on EVERY component through the base-class pointer.
    // The vtable dispatches to the correct derived implementation at runtime:
    //   Processor::refresh()   -> reads GetSystemTimes()
    //   Memory::refresh()      -> reads GlobalMemoryStatusEx()
    //   Battery::refresh()     -> reads GetSystemPowerStatus()
    //   GraphicsCard::refresh() -> no-op (needs vendor SDK)
    // -------------------------------------------------------------------------
    void update() {
        ++simulationTick;

        // [POLYMORPHISM] Virtual dispatch — one loop, four behaviours
        for (auto& comp : dataManager.getComponents()) {
            comp->refresh();                        // POLYMORPHIC CALL
        }

        // Record CPU history
        if (dataManager.getCPU()) {
            cpuHistory.push_back(dataManager.getCPU()->getUsagePercent());
            if (cpuHistory.size() > 10) cpuHistory.erase(cpuHistory.begin());
        }

        // Refresh real process list
        processList = SystemQuery::queryProcesses(15);
    }

    // -- Print REAL process table ----------------------------------------------
    void printProcessTable() const {
        HardwareComponent::printSeparator('=');
        std::cout << "  RUNNING PROCESSES  (Top 15 by Memory)\n";
        HardwareComponent::printSeparator();
        std::cout << "  " << std::left
                  << std::setw(8)  << "PID"
                  << std::setw(30) << "Process Name"
                  << std::setw(14) << "RAM (MB)"
                  << "\n";
        HardwareComponent::printSeparator('-');
        for (const auto& p : processList) {
            std::cout << "  " << std::left
                      << std::setw(8)  << p.pid
                      << std::setw(30) << p.name
                      << std::fixed << std::setprecision(1)
                      << std::setw(14) << p.memoryMB
                      << "\n";
        }
        HardwareComponent::printSeparator('=');
    }

    // -- Print statistics summary -----------------------------------------------
    void printStatsSummary() const {
        double avgCPU = cpuHistory.empty() ? 0.0
            : std::accumulate(cpuHistory.begin(), cpuHistory.end(), 0.0)
              / cpuHistory.size();
        double peakCPU = cpuHistory.empty() ? 0.0
            : *std::max_element(cpuHistory.begin(), cpuHistory.end());

        HardwareComponent::printSeparator('=');
        std::cout << "  SYSTEM STATISTICS  (last "
                  << cpuHistory.size() << " ticks)\n";
        HardwareComponent::printSeparator();
        if (dataManager.getCPU())
            HardwareComponent::printRow("CPU Usage Now",
                [&]{ std::ostringstream o;
                     o << std::fixed << std::setprecision(1)
                       << dataManager.getCPU()->getUsagePercent();
                     return o.str(); }() + " %");
        HardwareComponent::printRow("Avg CPU (history)",
            [&]{ std::ostringstream o;
                 o << std::fixed << std::setprecision(1) << avgCPU;
                 return o.str(); }() + " %");
        HardwareComponent::printRow("Peak CPU (history)",
            [&]{ std::ostringstream o;
                 o << std::fixed << std::setprecision(1) << peakCPU;
                 return o.str(); }() + " %");
        if (dataManager.getRAM())
            HardwareComponent::printRow("RAM Usage",
                std::to_string(dataManager.getRAM()->getUsagePercent()) + " %");
        if (dataManager.getBattery() && dataManager.getBattery()->getHasBattery())
            HardwareComponent::printRow("Battery",
                std::to_string(dataManager.getBattery()->getChargePercent()) + " %");
        HardwareComponent::printSeparator('=');
    }

    // -- Quick-view: calls getUsageStats() polymorphically ---------------------
    void printAllUsageStats() const {
        HardwareComponent::printSeparator('=');
        std::cout << "  LIVE SYSTEM OVERVIEW\n";
        HardwareComponent::printSeparator();
        // [POLYMORPHISM] Each call dispatches to the correct derived version
        for (const auto& comp : dataManager.getComponents()) {
            std::cout << "\n  [ " << comp->getType() << " ]  "
                      << comp->getName() << "\n";
            HardwareComponent::printSeparator('-');
            std::cout << comp->getUsageStats();      // POLYMORPHIC CALL
        }
        HardwareComponent::printSeparator('=');
    }

    // -- [ENCAPSULATION] Getters -----------------------------------------------
    int getTick() const { return simulationTick; }

    // --- NEW ADDITION START --- (getter for dashboard sparkline)
    const std::vector<double>& getCPUHistory() const { return cpuHistory; }
    // --- NEW ADDITION END ---
};

#endif // PROCESSING_MODULE_H
