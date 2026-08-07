// =============================================================================
// FILE: Memory.h
// PURPOSE: Derived class for RAM — reads REAL memory stats from Windows.
//          Demonstrates INHERITANCE, ENCAPSULATION, and POLYMORPHISM.
// =============================================================================

#pragma once
#ifndef MEMORY_H
#define MEMORY_H

#include "HardwareComponent.h"
#include "SystemQuery.h"
#include <sstream>

// -----------------------------------------------------------------------------
// [INHERITANCE] Memory IS-A HardwareComponent.
// -----------------------------------------------------------------------------
class Memory : public HardwareComponent {
    // -- [ENCAPSULATION] Private RAM Attributes --------------------------------
private:
    uint64_t totalCapacity_MB;
    uint64_t usedCapacity_MB;
    uint64_t availableCapacity_MB;
    int      usagePercent;

public:
    // -- Constructor: queries REAL RAM via GlobalMemoryStatusEx -----------------
    Memory()
        : HardwareComponent("System Memory", "RAM", "System")
    {
        SystemQuery::MemInfo info = SystemQuery::queryMemory();
        totalCapacity_MB     = info.totalPhysical_MB;
        usedCapacity_MB      = info.usedPhysical_MB;
        availableCapacity_MB = info.availPhysical_MB;
        usagePercent         = info.usagePercent;
    }

    // -- [POLYMORPHISM] Override: getUsageStats() ------------------------------
    std::string getUsageStats() const override {
        std::ostringstream oss;
        oss << "  Total RAM    : " << totalCapacity_MB << " MB ("
            << totalCapacity_MB / 1024 << " GB)\n";
        oss << "  Used         : " << usedCapacity_MB  << " MB ("
            << usedCapacity_MB / 1024 << " GB)\n";
        oss << "  Available    : " << availableCapacity_MB << " MB ("
            << availableCapacity_MB / 1024 << " GB)\n";
        oss << "  Usage        : " << usagePercent << " %\n";
        return oss.str();
    }

    // -- [POLYMORPHISM] Override: displayDetails() -----------------------------
    void displayDetails() const override {
        int barFill = usagePercent / 5;

        printSeparator('=');
        std::cout << "  MEMORY (RAM)\n";
        printSeparator();
        printRow("Total Capacity",
                 std::to_string(totalCapacity_MB) + " MB (" +
                 std::to_string(totalCapacity_MB / 1024) + " GB)");
        printRow("Used",
                 std::to_string(usedCapacity_MB) + " MB (" +
                 std::to_string(usedCapacity_MB / 1024) + " GB)");
        printRow("Available",
                 std::to_string(availableCapacity_MB) + " MB (" +
                 std::to_string(availableCapacity_MB / 1024) + " GB)");
        printRow("Usage",          std::to_string(usagePercent) + " %");
        printSeparator();
        // Visual capacity bar
        std::cout << "  Usage  ["
                  << std::string(barFill, '#')
                  << std::string(20 - barFill, '.')
                  << "] " << usagePercent << " %\n";
        printSeparator('=');
    }

    // -- [POLYMORPHISM] Override: refresh() ------------------------------------
    // Re-reads REAL memory stats from Windows each tick.
    // -------------------------------------------------------------------------
    void refresh() override {
        SystemQuery::MemInfo info = SystemQuery::queryMemory();
        usedCapacity_MB      = info.usedPhysical_MB;
        availableCapacity_MB = info.availPhysical_MB;
        usagePercent         = info.usagePercent;
    }

    // -- [ENCAPSULATION] Getters -----------------------------------------------
    uint64_t getTotalCapacity_MB()  const { return totalCapacity_MB;     }
    uint64_t getUsedCapacity_MB()   const { return usedCapacity_MB;      }
    uint64_t getAvailable_MB()      const { return availableCapacity_MB; }
    int      getUsagePercent()      const { return usagePercent;         }
};

#endif // MEMORY_H
