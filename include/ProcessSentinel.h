// =============================================================================
// FILE: ProcessSentinel.h
// PURPOSE: Interactive Process Sentinel module — demonstrates ABSTRACTION,
//          INHERITANCE, POLYMORPHISM, and ENCAPSULATION through:
//            1. SystemAction      — abstract base class (pure virtual)
//            2. InteractiveKillAction — derived class (polymorphic override)
//            3. ThresholdManager  — encapsulated threshold configuration
// --- NEW ADDITION START ---
// =============================================================================

#pragma once
#ifndef PROCESS_SENTINEL_H
#define PROCESS_SENTINEL_H

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>

#include "SystemQuery.h"
#include "Memory.h"
#include "DashboardRenderer.h"

// =============================================================================
// [ABSTRACTION] SystemAction — abstract base class.
// Defines a contract that all system actions must follow.
// Cannot be instantiated directly due to the pure virtual execute() method.
// =============================================================================
class SystemAction {
    // -- [ENCAPSULATION] Protected attributes accessible to derived classes ---
protected:
    std::string actionName;
    std::string description;

public:
    // -- Constructor / Virtual Destructor --------------------------------------
    SystemAction(const std::string& name, const std::string& desc)
        : actionName(name), description(desc) {}

    // Virtual destructor — required for safe polymorphic deletion via
    // base-class pointers.
    virtual ~SystemAction() = default;

    // -- [ABSTRACTION] Pure Virtual Method -------------------------------------
    // This MUST be overridden by every concrete derived class.
    // It defines WHAT an action does without specifying HOW.
    // -------------------------------------------------------------------------
    virtual void execute() = 0;

    // -- [ENCAPSULATION] Public Getters ----------------------------------------
    std::string getName()        const { return actionName;  }
    std::string getDescription() const { return description; }
};

// =============================================================================
// [INHERITANCE] InteractiveKillAction IS-A SystemAction.
// [POLYMORPHISM] Overrides execute() to provide specific kill behaviour.
// When triggered, it renders a high-visibility alert, pauses the dashboard,
// prompts the user for confirmation, and optionally kills the target process.
// =============================================================================
class InteractiveKillAction : public SystemAction {
    // -- [ENCAPSULATION] Private State ----------------------------------------
private:
    std::string targetProcess;   // Name of the process to kill (e.g. "chrome.exe")
    double      targetMemoryMB;  // Memory usage of the target (for display)

public:
    // -- Constructor: calls base-class constructor (INHERITANCE) ---------------
    InteractiveKillAction()
        : SystemAction("Kill Process",
                        "Terminate the top memory-consuming process"),
          targetProcess(""), targetMemoryMB(0.0) {}

    // -- [ENCAPSULATION] Setters with controlled access -----------------------
    void setTarget(const std::string& procName, double memMB) {
        targetProcess  = procName;
        targetMemoryMB = memMB;
    }

    // -- [POLYMORPHISM] Override: execute() ------------------------------------
    // This is the polymorphic method. When called through a SystemAction*,
    // the vtable dispatches to THIS implementation at runtime.
    // -------------------------------------------------------------------------
    void execute() override {
        if (targetProcess.empty()) return;

        // Show the console cursor for user input
        DashboardRenderer::showConsoleCursor();

        // Render the high-visibility alert overlay
        DashboardRenderer::renderAlertBox(targetProcess, targetMemoryMB);

        // Wait for Y/N input
        char response = ' ';
        while (response != 'Y' && response != 'y' &&
               response != 'N' && response != 'n') {
            if (_kbhit()) {
                response = (char)_getch();
            }
            Sleep(50);
        }

        bool killed = false;
        if (response == 'Y' || response == 'y') {
            // Build and execute taskkill command
            std::string cmd = "taskkill /F /IM " + targetProcess + " >nul 2>&1";
            int result = system(cmd.c_str());
            killed = (result == 0);
        }

        // Show result feedback
        DashboardRenderer::renderKillResult(killed, targetProcess);

        // Hide cursor again for dashboard mode
        DashboardRenderer::hideConsoleCursor();

        // Reset target after action
        targetProcess  = "";
        targetMemoryMB = 0.0;
    }

    // -- [ENCAPSULATION] Getter -----------------------------------------------
    std::string getTargetProcess() const { return targetProcess;  }
    double      getTargetMemory()  const { return targetMemoryMB; }
};

// =============================================================================
// [ENCAPSULATION] ThresholdManager — manages alert thresholds with private
// data members and controlled getters/setters.
// External code cannot directly access or tamper with threshold values.
// =============================================================================
class ThresholdManager {
    // -- [ENCAPSULATION] Private Variables -------------------------------------
    // These cannot be accessed or modified from outside the class.
    // All access goes through validated getters/setters below.
    // -------------------------------------------------------------------------
private:
    double maxSafeRamLimit;    // Maximum safe RAM usage before triggering alert
    bool   alertFired;         // Prevents alert spam — fires once per breach

public:
    // -- Constructor: initializes with a safe default -------------------------
    ThresholdManager() : maxSafeRamLimit(90.0), alertFired(false) {}

    // -- [ENCAPSULATION] Secure Getter ----------------------------------------
    // Read-only access to the private threshold value.
    double getMaxSafeRamLimit() const { return maxSafeRamLimit; }

    // -- [ENCAPSULATION] Secure Setter ----------------------------------------
    // Validates input and clamps to a safe range [10.0 .. 99.0].
    // Prevents nonsensical thresholds (e.g., 0% or 150%).
    void setMaxSafeRamLimit(double limit) {
        if (limit < 10.0)  limit = 10.0;
        if (limit > 99.0)  limit = 99.0;
        maxSafeRamLimit = limit;
        alertFired = false;  // Reset alert state on threshold change
    }

    // -- Breach Detection -----------------------------------------------------
    // Compares live RAM usage against the encapsulated threshold.
    bool isBreached(const Memory* ram) const {
        if (!ram) return false;
        return (double)ram->getUsagePercent() >= maxSafeRamLimit;
    }

    // -- Alert State Management -----------------------------------------------
    bool hasAlertFired()        const { return alertFired; }
    void markAlertFired()             { alertFired = true; }
    void resetAlert()                 { alertFired = false; }

    // -- Identify Top Offender ------------------------------------------------
    // Returns the name and memory of the process using the most RAM.
    // The process list from SystemQuery is already sorted by memory desc.
    static std::string getTopOffenderName(
        const std::vector<SystemQuery::ProcessEntry>& procs)
    {
        if (procs.empty()) return "N/A";
        return procs[0].name;
    }

    static double getTopOffenderMB(
        const std::vector<SystemQuery::ProcessEntry>& procs)
    {
        if (procs.empty()) return 0.0;
        return procs[0].memoryMB;
    }

    // -- Interactive Configuration UI -----------------------------------------
    // Displays current threshold and prompts for a new value.
    void configureInteractive() {
        DashboardRenderer::showConsoleCursor();
        system("cls");

        std::cout << "\n";
        std::cout << "  +==========================================================+\n";
        std::cout << "  |           SENTINEL THRESHOLD CONFIGURATION               |\n";
        std::cout << "  +==========================================================+\n";
        std::cout << "\n";
        std::cout << "  Current RAM Threshold : " << std::fixed
                  << std::setprecision(1) << maxSafeRamLimit << " %\n";
        std::cout << "  Valid Range           : 10.0 % — 99.0 %\n";
        std::cout << "\n";
        std::cout << "  Enter new threshold (or 0 to keep current): ";

        double newVal = 0.0;
        std::cin >> newVal;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (newVal > 0.0) {
            setMaxSafeRamLimit(newVal);
            std::cout << "\n  [OK] Threshold updated to "
                      << std::fixed << std::setprecision(1)
                      << maxSafeRamLimit << " %\n";
        } else {
            std::cout << "\n  [--] Threshold unchanged.\n";
        }

        std::cout << "\n  Press ENTER to return to menu...";
        std::cin.get();
    }
};

#endif // PROCESS_SENTINEL_H
// --- NEW ADDITION END ---
