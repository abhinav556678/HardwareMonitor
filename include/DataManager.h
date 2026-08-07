// =============================================================================
// FILE: DataManager.h
// PURPOSE: Data Management Module — detects REAL hardware and owns all
//          component objects. Demonstrates ENCAPSULATION and POLYMORPHISM.
// =============================================================================

#pragma once
#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "HardwareComponent.h"
#include "SystemQuery.h"
#include "Processor.h"
#include "GraphicsCard.h"
#include "Memory.h"
#include "Battery.h"

#include <vector>
#include <memory>
#include <string>
#include <iostream>

// =============================================================================
// [ENCAPSULATION] DataManager owns all hardware profiles privately.
// [POLYMORPHISM]  Stores derived objects via base-class unique_ptr.
// =============================================================================
class DataManager {
private:
    std::vector<std::unique_ptr<HardwareComponent>> components;

    // Non-owning typed pointers for direct access
    Processor*    cpu     = nullptr;
    GraphicsCard* gpu     = nullptr;
    Memory*       ram     = nullptr;
    Battery*      battery = nullptr;

    std::string systemName;
    std::string osVersion;

public:
    DataManager() {
        systemName = SystemQuery::getComputerName();
        osVersion  = SystemQuery::getOSVersion();
        detectHardware();
    }

    // -- [DATA MANAGEMENT MODULE] detectHardware() -----------------------------
    // Creates component objects with REAL system data detected at runtime.
    // Each component queries the appropriate Windows API in its constructor.
    // -------------------------------------------------------------------------
    void detectHardware() {
        // CPU — reads CPUID + registry
        auto cpuPtr = std::make_unique<Processor>();
        cpu = cpuPtr.get();
        components.push_back(std::move(cpuPtr));

        // GPU — reads DXGI
        auto gpuPtr = std::make_unique<GraphicsCard>();
        gpu = gpuPtr.get();
        components.push_back(std::move(gpuPtr));

        // RAM — reads GlobalMemoryStatusEx
        auto ramPtr = std::make_unique<Memory>();
        ram = ramPtr.get();
        components.push_back(std::move(ramPtr));

        // Battery — reads GetSystemPowerStatus
        auto batPtr = std::make_unique<Battery>();
        battery = batPtr.get();
        components.push_back(std::move(batPtr));
    }

    // -- [ENCAPSULATION] Public Getters ----------------------------------------
    const std::vector<std::unique_ptr<HardwareComponent>>& getComponents() const {
        return components;
    }
    Processor*    getCPU()     const { return cpu;     }
    GraphicsCard* getGPU()     const { return gpu;     }
    Memory*       getRAM()     const { return ram;     }
    Battery*      getBattery() const { return battery; }
    std::string   getSystemName() const { return systemName; }
    std::string   getOSVersion()  const { return osVersion;  }

    void printSystemProfile() const {
        std::cout << "\n";
        HardwareComponent::printSeparator('=');
        std::cout << "  SYSTEM PROFILE\n";
        HardwareComponent::printSeparator();
        HardwareComponent::printRow("Computer Name",  systemName);
        HardwareComponent::printRow("OS",             osVersion);
        HardwareComponent::printRow("Components",
            std::to_string(components.size()) + " detected");
        HardwareComponent::printSeparator('=');
        std::cout << "\n";
    }
};

#endif // DATA_MANAGER_H
