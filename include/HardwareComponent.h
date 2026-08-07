// =============================================================================
// FILE: HardwareComponent.h
// PURPOSE: Abstract Base Class — demonstrates ABSTRACTION and sets the
//          contract for all derived hardware component classes.
// =============================================================================

#pragma once
#ifndef HARDWARE_COMPONENT_H
#define HARDWARE_COMPONENT_H

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

class HardwareComponent {
public:

protected:
    std::string componentName;   // Human-readable name (e.g., "AMD Ryzen 7")
    std::string componentType;   // Category label (e.g., "CPU", "GPU")
    std::string manufacturer;    // Vendor name
    bool        isActive;        // Whether this component is currently powered

public:
    HardwareComponent(const std::string& name,
                      const std::string& type,
                      const std::string& mfr)
        : componentName(name), componentType(type),
          manufacturer(mfr), isActive(true) {}

    virtual ~HardwareComponent() = default;

    // Returns a formatted multi-line string of current usage/health statistics.
    virtual std::string getUsageStats() const = 0;

    // Prints a detailed information panel for this component.
    virtual void displayDetails() const = 0;

    // Refreshes live hardware stats from the real system (Win32 APIs).
    virtual void refresh() = 0;

    std::string getName()         const { return componentName; }
    std::string getType()         const { return componentType; }
    std::string getManufacturer() const { return manufacturer;  }
    bool        getIsActive()     const { return isActive;      }

    // -- [ENCAPSULATION] Public Setters ----------------------------------------
    void setActive(bool state) { isActive = state; }

    // -- Shared Utility: renders a horizontal separator bar --------------------
    // Accepts a plain ASCII char (default '-' rendered as '-') to avoid MSVC
    // C4305 truncation warnings from multi-byte UTF-8 box-drawing characters.
    static void printSeparator(char ch = '-', int width = 60) {
        std::cout << std::string(width, ch) << "\n";
    }

    // Overload: separator using a full string token for styled output
    static void printSeparator(const std::string& token, int repeat = 60) {
        std::string line;
        line.reserve(token.size() * (size_t)repeat);
        for (int i = 0; i < repeat; ++i) line += token;
        std::cout << line.substr(0, 60 * 3) << "\n"; // cap to ~60 display cols
    }

    // Shared utility: renders a formatted label-value row
    static void printRow(const std::string& label,
                         const std::string& value,
                         int labelWidth = 28) {
        std::cout << "  " << std::left << std::setw(labelWidth)
                  << label << ": " << value << "\n";
    }
};

#endif // HARDWARE_COMPONENT_H
