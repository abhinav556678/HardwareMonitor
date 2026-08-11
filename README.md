# Hardware Resource & Task Monitor — v3.0

A terminal-based system monitor written in C++17, built entirely on raw Win32 APIs with zero external dependencies. It reads real hardware data directly from the kernel — CPU, GPU, RAM, battery — and renders a live, flicker-free dashboard right inside your console window.

The whole point of this project was to build something that actually works the way monitors should: fast, lightweight, and honest about what it's reading. No WMI overhead, no .NET runtime, no Electron window pretending to be a tool.

---

## The RAM Irony Problem

Most GUI-based system monitors consume 20–50 MB of RAM while telling you how much RAM you're using. This project was partly built to fix that irony.

| | Hardware Monitor v3.0 | Task Manager | Process Explorer |
|---|---|---|---|
| RAM footprint | ~2 MB | ~35 MB | ~22 MB |
| CPU overhead | < 0.1% | ~0.5% | ~0.3% |
| External dependencies | None | Bundled | Bundled |
| Startup time | < 50 ms | ~800 ms | ~600 ms |
| Executable size | ~480 KB | System | ~2 MB |
| Works over SSH / terminal | Yes | No | No |

Keeping it this lean is intentional. There are no background threads polling sensors, no heap allocations in the render loop, and no GUI subsystem loaded at all. The application sleeps for exactly 1 second between frames using `Sleep(1000)` and lets the OS scheduler do the rest.

---

## What It Actually Does

### Live Dashboard — flicker-free, 1-second refresh

This is the main feature. Pressing `7` from the menu enters a full-screen TUI mode with a 120-column layout showing CPU, RAM, GPU, and battery in real time. The dashboard also shows a two-column split: the process sentinel on the left and the top memory-consuming processes on the right.

What makes it work without flickering: instead of calling `system("cls")` which blanks the whole screen and redraws from scratch, the renderer calls `SetConsoleCursorPosition()` before writing each section. This overwrites only the characters that need updating. The screen never goes blank, so there is no flash between frames. This is the same approach htop and top use on Linux — just done with Win32 instead of ncurses.

The progress bars use DOS block characters (`█`, `░`) to fill a 40-character bar, and the CPU has a scrolling sparkline history (20 ticks) built from `░▒▓█`. The header shows a live system health score from 0 to 100, the current time, and the session uptime. Colors shift from normal to yellow to red as usage crosses 70% and 90% thresholds.

### Process Sentinel — memory watchdog with interactive kill

You can configure a RAM threshold anywhere from 10% to 99%. Once live RAM crosses that line, the sentinel triggers. It identifies which process is using the most memory, renders a high-visibility alert banner over the dashboard, and asks whether you want to terminate it. If you say yes, it runs `taskkill /F /IM <process>` and reports back. If you say no, it dismisses and resumes monitoring. The alert fires only once per breach and resets automatically when RAM drops back below the threshold — so it doesn't spam you.

### Real hardware data — no stubs, no guessing

Every number shown is read directly from the operating system:

| What | How |
|---|---|
| CPU model and core count | `__cpuid` intrinsic + `GetLogicalProcessorInformation` |
| CPU clock speed | Windows Registry at `HARDWARE\DESCRIPTION\System\CentralProcessor\0` |
| Live CPU usage % | `GetSystemTimes` — idle/kernel/user delta computed each tick |
| GPU name and VRAM | DXGI — `CreateDXGIFactory` → `IDXGIAdapter::GetDesc` |
| RAM total, used, available | `GlobalMemoryStatusEx` |
| Battery charge and state | `GetSystemPowerStatus` |
| Running processes and their memory | `CreateToolhelp32Snapshot` + `GetProcessMemoryInfo` |
| OS version and computer name | Windows Registry at `SOFTWARE\Microsoft\Windows NT\CurrentVersion` |

The CPU usage calculation deserves a note. The raw counter values from `GetSystemTimes` are meaningless on their own — they are cumulative since boot. What matters is the delta between two consecutive reads:

```
usage % = (1 - delta_idle / (delta_kernel + delta_user)) * 100
```

The first call seeds a static baseline. Every subsequent call computes the delta and updates the baseline. This is exactly how Task Manager calculates CPU%.

For the GPU, it uses the DirectX Graphics Infrastructure COM interface — the same one games and driver software use — rather than registry hacks or WMI:

```cpp
CreateDXGIFactory(__uuidof(IDXGIFactory), &pFactory);
pFactory->EnumAdapters(i, &pAdapter);
pAdapter->GetDesc(&desc);  // gives name, dedicated VRAM, shared memory
```

### Diagnostic report export

Option `9` from the menu generates `system_report.txt` in the same folder as the executable. It writes a timestamped, section-by-section report covering the computer name, OS version, CPU specs, GPU specs, RAM state, battery state, and the top 10 processes sorted by memory. Straightforward file I/O with `std::ofstream`, no third-party formatters.

### Authentication gate

The application opens behind a login screen. The user database is stored in a private member of `LoginModule` — nothing outside the class can read or write it. This is a small thing but it makes the encapsulation story concrete: `authenticate()` is the only public surface.

### Animated splash screen

On startup, the ASCII art banner is printed character by character with a small delay per character. It gives the launch a polished feel without adding any real complexity.

---

## OOP Design

This project was also built to demonstrate all four pillars of object-oriented programming in genuinely working code — not toy examples.

**Abstraction** lives in two abstract base classes that cannot be instantiated:

- `HardwareComponent` — defines the interface every hardware object must implement (`refresh()`, `displayDetails()`, `getUsageStats()`). The pure virtual `refresh()` forces every subclass to provide its own data-fetch logic.
- `SystemAction` — defines what a "system action" is via a pure virtual `execute()`. Any future action (log, notify, restart) just inherits from this and overrides `execute()`.

**Inheritance** connects the components:

```
HardwareComponent  (abstract)
    ├── Processor        — reads CPU via CPUID + GetSystemTimes
    ├── GraphicsCard     — reads GPU via DXGI
    ├── Memory           — reads RAM via GlobalMemoryStatusEx
    └── Battery          — reads battery via GetSystemPowerStatus

SystemAction       (abstract)
    └── InteractiveKillAction  — shows alert, prompts, calls taskkill
```

**Polymorphism** is where it comes together. `ProcessingModule::update()` holds a `vector<HardwareComponent*>` and calls `comp->refresh()` on each. The vtable dispatches to the correct subclass at runtime — one call site, four different read implementations. The sentinel uses the same pattern:

```cpp
SystemAction* action = &killAction;
action->execute();  // vtable dispatch — goes to InteractiveKillAction::execute()
```

**Encapsulation** is enforced throughout:

- `ThresholdManager` keeps `maxSafeRamLimit` private. The setter clamps input to the range `[10.0, 99.0]` and rejects anything outside it. External code cannot set an invalid threshold.
- `DataManager` owns all hardware component objects but exposes them only through typed getter methods. The storage vector is private.
- `LoginModule` keeps its credential store completely hidden. `authenticate()` is the only way in.

---

## Project Structure

```
HardwareMonitor/
├── src/
│   └── main.cpp                 — entry point, menu loop, live dashboard runner
│
├── include/
│   ├── HardwareComponent.h      — abstract base class for all hardware types
│   ├── Processor.h              — CPU data (CPUID, GetSystemTimes)
│   ├── GraphicsCard.h           — GPU data (DXGI)
│   ├── Memory.h                 — RAM data (GlobalMemoryStatusEx)
│   ├── Battery.h                — battery data (GetSystemPowerStatus)
│   ├── DataManager.h            — owns hardware objects, typed getters
│   ├── ProcessingModule.h       — drives polymorphic refresh(), holds CPU history
│   ├── SystemQuery.h            — static Win32 API wrappers (the data layer)
│   ├── DashboardRenderer.h      — TUI engine: cursor positioning, color, bars, layout
│   ├── ProcessSentinel.h        — SystemAction + InteractiveKillAction + ThresholdManager
│   ├── ReportGenerator.h        — ofstream-based diagnostic report writer
│   ├── LoginModule.h            — credential authentication gate
│   └── SplashScreen.h          — animated ASCII intro
│
└── CMakeLists.txt               — C++17, MSVC /W4, links dxgi + advapi32 + psapi
```

13 headers, 1 source file, around 1,500 lines of C++17, built with MSVC x64.

---

## Building

**Requirements:** Windows 10 or 11 (x64), Visual Studio 2019+ or standalone MSVC build tools, CMake 3.16+.

```powershell
git clone https://github.com/abhinav556678/HardwareMonitor.git
cd HardwareMonitor

cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

.\build\Release\HardwareMonitor.exe
```

If you just want to compile quickly from an MSVC developer prompt:

```
cl /std:c++17 /O2 /I include src\main.cpp /link dxgi.lib advapi32.lib psapi.lib /out:HardwareMonitor.exe
```

Visual Studio will also auto-detect the `CMakeLists.txt` if you open the folder directly.

---

## Menu Reference

```
[1]  Live System Overview
[2]  Processor (CPU) Details
[3]  Graphics Card (GPU) Details
[4]  Memory (RAM) Details
[5]  Battery Status
[6]  Running Processes
[7]  Live Dashboard Mode  (TUI)
[8]  Configure Sentinel Threshold
[9]  Export System Report
[10] About / OOP Concepts
[0]  Exit
```

---

## What Could Be Added Next

- Per-core CPU usage breakdown
- Network throughput per adapter
- Disk read/write speeds via `NtQuerySystemInformation`
- CPU temperature (needs WMI or manufacturer-specific driver access)
- Log-to-CSV mode for time-series graphs
- Multi-GPU display (currently picks the adapter with the most VRAM)

---

## Platform

Windows-only by design. The project depends on Win32, DXGI, and PSAPI — none of which exist on Linux or macOS. A cross-platform version would need to replace the entire `SystemQuery.h` layer with platform-specific equivalents (`/proc/stat` for CPU on Linux, Metal Performance Shaders for GPU on macOS, etc.).

---

Built with C++17. No frameworks, no runtime bloat.
