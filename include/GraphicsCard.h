// =============================================================================
// FILE: GraphicsCard.h
// PURPOSE: Derived class for GPU — reads REAL GPU info via DXGI.
//          Demonstrates INHERITANCE, ENCAPSULATION, and POLYMORPHISM.
// =============================================================================

#pragma once
#ifndef GRAPHICS_CARD_H
#define GRAPHICS_CARD_H

#include "HardwareComponent.h"
#include "SystemQuery.h"
#include <sstream>

// -----------------------------------------------------------------------------
// [INHERITANCE] GraphicsCard IS-A HardwareComponent.
// -----------------------------------------------------------------------------
class GraphicsCard : public HardwareComponent {
    // -- [ENCAPSULATION] Private GPU-Specific Attributes -----------------------
private:
    size_t dedicatedVRAM_MB;
    size_t sharedMemory_MB;

public:
    // -- Constructor: queries REAL GPU via DXGI --------------------------------
    GraphicsCard()
        : HardwareComponent("", "GPU", "")
    {
        SystemQuery::GPUInfo info = SystemQuery::queryGPU();
        componentName    = info.name;
        dedicatedVRAM_MB = info.dedicatedVRAM_MB;
        sharedMemory_MB  = info.sharedMemory_MB;

        // Detect manufacturer from GPU name
        if (componentName.find("NVIDIA") != std::string::npos ||
            componentName.find("GeForce") != std::string::npos)
            manufacturer = "NVIDIA";
        else if (componentName.find("AMD") != std::string::npos ||
                 componentName.find("Radeon") != std::string::npos)
            manufacturer = "AMD";
        else if (componentName.find("Intel") != std::string::npos)
            manufacturer = "Intel";
        else
            manufacturer = "Unknown";
    }

    // -- [POLYMORPHISM] Override: getUsageStats() ------------------------------
    std::string getUsageStats() const override {
        std::ostringstream oss;
        oss << "  GPU Name     : " << componentName << "\n";
        oss << "  Dedicated    : " << dedicatedVRAM_MB << " MB\n";
        oss << "  Shared Mem   : " << sharedMemory_MB  << " MB\n";
        oss << "  (Live GPU load requires vendor SDK - NVML/ADL)\n";
        return oss.str();
    }

    // -- [POLYMORPHISM] Override: displayDetails() -----------------------------
    void displayDetails() const override {
        printSeparator('=');
        std::cout << "  GRAPHICS CARD -- " << componentName << "\n";
        printSeparator();
        printRow("Manufacturer",     manufacturer);
        printRow("Model",            componentName);
        printRow("Dedicated VRAM",   std::to_string(dedicatedVRAM_MB) + " MB ("
                                     + std::to_string(dedicatedVRAM_MB / 1024) + " GB)");
        printRow("Shared Memory",    std::to_string(sharedMemory_MB) + " MB");
        printSeparator();
        std::cout << getUsageStats();
        printSeparator('=');
    }

    // -- [POLYMORPHISM] Override: refresh() ------------------------------------
    // GPU utilisation requires vendor-specific APIs (NVML for NVIDIA, ADL for AMD).
    // Static info (name, VRAM) is already populated from DXGI at construction.
    // -------------------------------------------------------------------------
    void refresh() override {
        // No-op: real-time GPU load/temp requires NVML or ADL SDK
    }

    // -- [ENCAPSULATION] Getters -----------------------------------------------
    size_t getDedicatedVRAM_MB() const { return dedicatedVRAM_MB; }
    size_t getSharedMemory_MB()  const { return sharedMemory_MB;  }
};

#endif // GRAPHICS_CARD_H
