// =============================================================================
// FILE: Battery.h
// PURPOSE: Derived class for Battery — reads REAL battery state from Windows.
//          Demonstrates INHERITANCE, ENCAPSULATION, and POLYMORPHISM.
//          Gracefully handles desktops (no battery present).
// =============================================================================

#pragma once
#ifndef BATTERY_H
#define BATTERY_H

#include "HardwareComponent.h"
#include "SystemQuery.h"
#include <sstream>

// -----------------------------------------------------------------------------
// [INHERITANCE] Battery IS-A HardwareComponent.
// -----------------------------------------------------------------------------
class Battery : public HardwareComponent {
    // -- [ENCAPSULATION] Private Battery Attributes ----------------------------
private:
    bool hasBattery;
    int  chargePercent;     // 0-100, or -1 if unknown
    bool isCharging;
    bool acPluggedIn;

public:
    // -- Constructor: queries REAL battery via GetSystemPowerStatus -------------
    Battery()
        : HardwareComponent("System Battery", "Battery", "System")
    {
        SystemQuery::BatteryInfo info = SystemQuery::queryBattery();
        hasBattery    = info.hasBattery;
        chargePercent = info.chargePercent;
        isCharging    = info.isCharging;
        acPluggedIn   = info.acPluggedIn;
    }

    // -- [POLYMORPHISM] Override: getUsageStats() ------------------------------
    std::string getUsageStats() const override {
        std::ostringstream oss;
        if (!hasBattery) {
            oss << "  No battery detected (desktop PC).\n";
            return oss.str();
        }
        std::string status = isCharging ? "Charging"
                           : acPluggedIn ? "Plugged In (Full)"
                           : "Discharging";
        oss << "  Charge       : "
            << (chargePercent >= 0 ? std::to_string(chargePercent) + " %" : "Unknown")
            << "  (" << status << ")\n";
        oss << "  AC Adapter   : " << (acPluggedIn ? "Connected" : "Disconnected") << "\n";
        return oss.str();
    }

    // -- [POLYMORPHISM] Override: displayDetails() -----------------------------
    void displayDetails() const override {
        printSeparator('=');
        std::cout << "  BATTERY\n";
        printSeparator();
        if (!hasBattery) {
            std::cout << "  No battery detected — this is a desktop PC.\n";
            printSeparator('=');
            return;
        }
        printRow("Status", isCharging ? "Charging"
                         : acPluggedIn ? "Plugged In" : "On Battery");
        printRow("AC Adapter", acPluggedIn ? "Connected" : "Disconnected");
        if (chargePercent >= 0) {
            printRow("Charge Level", std::to_string(chargePercent) + " %");
            // Visual bar
            int barFill = chargePercent / 5;
            std::cout << "  Level  ["
                      << std::string(barFill, '#')
                      << std::string(20 - barFill, '.')
                      << "] " << chargePercent << " %\n";
        } else {
            printRow("Charge Level", "Unknown");
        }
        printSeparator('=');
    }

    // -- [POLYMORPHISM] Override: refresh() ------------------------------------
    // Re-reads REAL battery state from Windows each tick.
    // -------------------------------------------------------------------------
    void refresh() override {
        SystemQuery::BatteryInfo info = SystemQuery::queryBattery();
        chargePercent = info.chargePercent;
        isCharging    = info.isCharging;
        acPluggedIn   = info.acPluggedIn;
    }

    // -- [ENCAPSULATION] Getters -----------------------------------------------
    bool getHasBattery()    const { return hasBattery;    }
    int  getChargePercent() const { return chargePercent; }
    bool getIsCharging()    const { return isCharging;    }
    bool getAcPluggedIn()   const { return acPluggedIn;   }
};

#endif // BATTERY_H
