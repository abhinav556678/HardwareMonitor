// =============================================================================
// FILE: Processor.h
// PURPOSE: Derived class for CPU — reads REAL CPU info from the system.
//          Demonstrates INHERITANCE, ENCAPSULATION, and POLYMORPHISM.
// =============================================================================

#pragma once
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "HardwareComponent.h"
#include "SystemQuery.h"
#include <sstream>
#include <algorithm>

// -----------------------------------------------------------------------------
// [INHERITANCE] Processor IS-A HardwareComponent.
// -----------------------------------------------------------------------------
class Processor : public HardwareComponent {
    // -- [ENCAPSULATION] Private Attributes ------------------------------------
private:
    int    physicalCores;
    int    logicalProcessors;
    int    ratedSpeedMHz;
    double usagePercent;

public:
    // -- Constructor: queries REAL CPU via SystemQuery --------------------------
    // [INHERITANCE] Calls the base-class constructor with detected CPU name.
    // -------------------------------------------------------------------------
    Processor()
        : HardwareComponent("", "CPU", "")
    {
        SystemQuery::CPUInfo info = SystemQuery::queryCPU();
        componentName      = info.brandString;
        physicalCores      = info.physicalCores;
        logicalProcessors  = info.logicalProcessors;
        ratedSpeedMHz      = info.ratedSpeedMHz;
        usagePercent       = 0.0;

        // Extract manufacturer from brand string
        if (componentName.find("AMD") != std::string::npos)
            manufacturer = "AMD";
        else if (componentName.find("Intel") != std::string::npos)
            manufacturer = "Intel";
        else
            manufacturer = "Unknown";
    }

    // -- [POLYMORPHISM] Override: getUsageStats() ------------------------------
    std::string getUsageStats() const override {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);
        oss << "  CPU Usage    : " << usagePercent << " %\n";
        oss << "  Rated Speed  : " << ratedSpeedMHz << " MHz\n";
        return oss.str();
    }

    // -- [POLYMORPHISM] Override: displayDetails() -----------------------------
    void displayDetails() const override {
        printSeparator('=');
        std::cout << "  PROCESSOR -- " << componentName << "\n";
        printSeparator();
        printRow("Manufacturer",    manufacturer);
        printRow("Model",           componentName);
        printRow("Physical Cores",  std::to_string(physicalCores));
        printRow("Logical Procs",   std::to_string(logicalProcessors));
        printRow("Rated Speed",     std::to_string(ratedSpeedMHz) + " MHz");
        printSeparator();
        std::cout << getUsageStats();
        // Usage bar
        int barFill = (int)(usagePercent / 5);
        std::cout << "  Load   ["
                  << std::string(barFill, '#')
                  << std::string(20 - barFill, '.')
                  << "] " << (int)usagePercent << " %\n";
        printSeparator('=');
    }

    // -- [POLYMORPHISM] Override: refresh() ------------------------------------
    // Reads REAL CPU usage from Windows GetSystemTimes() API.
    // -------------------------------------------------------------------------
    void refresh() override {
        usagePercent = SystemQuery::getCPUUsagePercent();
    }

    // -- [ENCAPSULATION] Getters -----------------------------------------------
    int    getPhysicalCores()    const { return physicalCores;     }
    int    getLogicalProcs()     const { return logicalProcessors; }
    int    getRatedSpeedMHz()    const { return ratedSpeedMHz;     }
    double getUsagePercent()     const { return usagePercent;      }
};

#endif // PROCESSOR_H
