// =============================================================================
// FILE: main.cpp
// PURPOSE: Application entry point — wires together all modules.
//          Reads REAL hardware data from the system running this program.
// =============================================================================

#define _CRT_SECURE_NO_WARNINGS

#include "LoginModule.h"
#include "DataManager.h"
#include "ProcessingModule.h"

// --- NEW ADDITION START --- (includes for TUI + Sentinel + Splash + Report)
#include "SplashScreen.h"
#include "DashboardRenderer.h"
#include "ProcessSentinel.h"
#include "ReportGenerator.h"
// --- NEW ADDITION END ---

#include <iostream>
#include <string>
#include <limits>
#include <thread>
#include <chrono>

void     printMainMenu();
void     waitForEnter();
void     clearScreen();
int      getMenuChoice(int min, int max);
// --- NEW ADDITION START --- (Live Dashboard function declaration)
void     runLiveDashboard(ProcessingModule& processor, DataManager& dataManager,
                          ThresholdManager& threshMgr, InteractiveKillAction& killAction);
// --- NEW ADDITION END ---

int main() {

    // --- NEW ADDITION START --- (Animated Splash Screen)
    SplashScreen::show();
    // --- NEW ADDITION END ---

    // -- STEP 1: Authentication ------------------------------------------------
    LoginModule loginModule;
    if (!loginModule.authenticate()) {
        return 1;
    }

    // -- STEP 2: Detect REAL Hardware ------------------------------------------
    // DataManager queries Windows APIs to discover the actual CPU, GPU, RAM,
    // and battery installed on THIS machine.
    DataManager dataManager;

    // -- STEP 3: Processing Module ---------------------------------------------
    ProcessingModule processor(dataManager);

    // --- NEW ADDITION START --- (Sentinel Module Instantiation)
    // ThresholdManager encapsulates the RAM threshold with secure getters/setters.
    // InteractiveKillAction is a polymorphic SystemAction used when threshold breaches.
    ThresholdManager   threshMgr;
    InteractiveKillAction killAction;
    // --- NEW ADDITION END ---

    dataManager.printSystemProfile();
    waitForEnter();

    // -- STEP 4: Main Menu Loop ------------------------------------------------
    bool running = true;
    while (running) {
        clearScreen();
        printMainMenu();

        int choice = getMenuChoice(0, 10);  // --- MODIFIED: range extended from 6 to 10 ---

        switch (choice) {
        case 1:
            clearScreen();
            processor.update();
            processor.printAllUsageStats();
            processor.printStatsSummary();
            waitForEnter();
            break;

        case 2:
            clearScreen();
            processor.update();
            if (dataManager.getCPU())
                dataManager.getCPU()->displayDetails();
            waitForEnter();
            break;

        case 3:
            clearScreen();
            processor.update();
            if (dataManager.getGPU())
                dataManager.getGPU()->displayDetails();
            waitForEnter();
            break;

        case 4:
            clearScreen();
            processor.update();
            if (dataManager.getRAM())
                dataManager.getRAM()->displayDetails();
            waitForEnter();
            break;

        case 5:
            clearScreen();
            processor.update();
            if (dataManager.getBattery())
                dataManager.getBattery()->displayDetails();
            waitForEnter();
            break;

        case 6:
            clearScreen();
            processor.update();
            processor.printProcessTable();
            waitForEnter();
            break;

        // --- NEW ADDITION START --- (Live Dashboard Mode)
        case 7:
            runLiveDashboard(processor, dataManager, threshMgr, killAction);
            break;
        // --- NEW ADDITION END ---

        // --- NEW ADDITION START --- (Sentinel Threshold Configuration)
        case 8:
            threshMgr.configureInteractive();
            break;
        // --- NEW ADDITION END ---

        // --- NEW ADDITION START --- (Export System Report)
        case 9:
            ReportGenerator::generateInteractive(dataManager);
            break;
        // --- NEW ADDITION END ---

        // --- NEW ADDITION START --- (OOP Concepts Showcase)
        case 10:
            DashboardRenderer::renderAboutScreen();
            break;
        // --- NEW ADDITION END ---

        case 0:
            running = false;
            break;
        }
    }

    std::cout << "\n  +=========================================================+\n";
    std::cout <<   "  |       Session ended. Hardware monitor shut down.        |\n";
    std::cout <<   "  +=========================================================+\n\n";
    return 0;
}

// =============================================================================
// UI Helpers
// =============================================================================
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void waitForEnter() {
    std::cout << "\n  Press ENTER to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

void printMainMenu() {
    std::cout << "\n";
    std::cout << "  +==========================================================+\n";
    std::cout << "  |        HARDWARE RESOURCE & TASK MONITOR  v3.0            |\n";
    std::cout << "  |               (Real Hardware Detection)                  |\n";
    std::cout << "  +==========================================================+\n";
    std::cout << "  |                                                          |\n";
    std::cout << "  |  [1]  Live System Overview                               |\n";
    std::cout << "  |  [2]  Processor (CPU) Details                            |\n";
    std::cout << "  |  [3]  Graphics Card (GPU) Details                        |\n";
    std::cout << "  |  [4]  Memory (RAM) Details                               |\n";
    std::cout << "  |  [5]  Battery Status                                     |\n";
    std::cout << "  |  [6]  Running Processes                                  |\n";
    std::cout << "  |                                                          |\n";
    // --- NEW ADDITION START --- (New menu entries)
    std::cout << "  |  [7]  Live Dashboard Mode  (TUI)                         |\n";
    std::cout << "  |  [8]  Configure Sentinel Thresholds                      |\n";
    std::cout << "  |  [9]  Export System Report                               |\n";
    std::cout << "  | [10]  About / OOP Concepts                               |\n";
    std::cout << "  |                                                          |\n";
    // --- NEW ADDITION END ---
    std::cout << "  |  [0]  Exit                                               |\n";
    std::cout << "  |                                                          |\n";
    std::cout << "  +==========================================================+\n";
    std::cout << "\n  Enter choice: ";
}

int getMenuChoice(int minVal, int maxVal) {
    int choice = -1;
    while (true) {
        if (std::cin >> choice && choice >= minVal && choice <= maxVal) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return choice;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  Invalid input. Enter a number ("
                  << minVal << "-" << maxVal << "): ";
    }
}

// --- NEW ADDITION START --- (Live Dashboard Loop)
// =============================================================================
// runLiveDashboard() — enters a continuous, flicker-free refresh loop.
// Uses DashboardRenderer for cursor-positioned output (no screen clear).
// Integrates ThresholdManager to check for RAM breaches each tick.
// If breached, InteractiveKillAction::execute() is invoked polymorphically
// through a SystemAction* pointer (demonstrating POLYMORPHISM).
// =============================================================================
void runLiveDashboard(ProcessingModule& processor, DataManager& dataManager,
                      ThresholdManager& threshMgr, InteractiveKillAction& killAction) {

    DashboardRenderer::initConsole();
    DashboardRenderer::wipeTransition(40);
    DashboardRenderer::hideConsoleCursor();

    bool dashRunning = true;
    while (dashRunning) {
        // 1. Refresh all hardware data (POLYMORPHIC — virtual dispatch)
        processor.update();

        // 2. Get process list for sentinel and display
        std::vector<SystemQuery::ProcessEntry> procs = SystemQuery::queryProcesses(15);
        std::string topName = ThresholdManager::getTopOffenderName(procs);
        double      topMB   = ThresholdManager::getTopOffenderMB(procs);

        // 3. Check threshold breach
        bool breached = threshMgr.isBreached(dataManager.getRAM());

        // 4. Calculate system health score
        int health = DashboardRenderer::calcHealth(dataManager);

        // 5. Render dashboard zones (flicker-free cursor repositioning)
        int row = DashboardRenderer::renderHeader(health);
        row = DashboardRenderer::renderTelemetry(dataManager, processor.getCPUHistory(), row);
        row = DashboardRenderer::renderColumns(row, breached, threshMgr.getMaxSafeRamLimit(), topName, topMB, procs);
        DashboardRenderer::renderFooter(row, processor.getTick());

        // 6. If threshold is breached and alert hasn't fired yet — trigger sentinel
        if (breached && !threshMgr.hasAlertFired()) {
            threshMgr.markAlertFired();

            // [POLYMORPHISM] Call execute() through base-class pointer.
            // The vtable dispatches to InteractiveKillAction::execute().
            killAction.setTarget(topName, topMB);
            SystemAction* action = &killAction;  // base-class pointer
            action->execute();                    // POLYMORPHIC CALL

            // After action, clear and re-render
            system("cls");
            DashboardRenderer::hideConsoleCursor();
            continue;
        }

        // Reset alert if RAM drops below threshold
        if (!breached && threshMgr.hasAlertFired()) {
            threshMgr.resetAlert();
        }

        // 7. Check for quit key (Q or q)
        Sleep(1000);
        if (_kbhit()) {
            char key = (char)_getch();
            if (key == 'q' || key == 'Q') {
                dashRunning = false;
            }
        }
    }

    DashboardRenderer::showConsoleCursor();
    DashboardRenderer::resetColor();
    system("cls");
}
// --- NEW ADDITION END ---
