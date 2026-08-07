// =============================================================================
// FILE: SystemQuery.h
// PURPOSE: Queries REAL hardware info from Windows APIs (Win32, DXGI, Registry)
// =============================================================================

#pragma once
#ifndef SYSTEM_QUERY_H
#define SYSTEM_QUERY_H

#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <intrin.h>

#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "psapi.lib")

// =============================================================================
// SystemQuery — static utility class for real hardware data
// =============================================================================
class SystemQuery {
public:
    // ── Data transfer structs ─────────────────────────────────────────────────
    struct CPUInfo {
        std::string brandString;
        int physicalCores;
        int logicalProcessors;
        int ratedSpeedMHz;
    };

    struct GPUInfo {
        std::string name;
        size_t dedicatedVRAM_MB;
        size_t sharedMemory_MB;
    };

    struct MemInfo {
        uint64_t totalPhysical_MB;
        uint64_t usedPhysical_MB;
        uint64_t availPhysical_MB;
        int      usagePercent;
    };

    struct BatteryInfo {
        bool hasBattery;
        int  chargePercent;   // 0-100, or -1 if unknown
        bool isCharging;
        bool acPluggedIn;
    };

    struct ProcessEntry {
        DWORD       pid;
        std::string name;
        double      memoryMB;
    };

    // ── CPU Brand String via CPUID ────────────────────────────────────────────
    static std::string getCPUBrand() {
        int cpuInfo[4] = {0};
        char brand[49] = {0};

        __cpuid(cpuInfo, 0x80000000);
        unsigned maxExt = cpuInfo[0];
        if (maxExt < 0x80000004) return "Unknown CPU";

        for (unsigned i = 0x80000002; i <= 0x80000004; ++i) {
            __cpuid(cpuInfo, i);
            std::memcpy(brand + (i - 0x80000002) * 16, cpuInfo, 16);
        }
        // Trim leading spaces
        std::string s(brand);
        size_t start = s.find_first_not_of(' ');
        return (start != std::string::npos) ? s.substr(start) : s;
    }

    // ── Physical core count via GetLogicalProcessorInformation ────────────────
    static int getPhysicalCoreCount() {
        DWORD len = 0;
        GetLogicalProcessorInformation(nullptr, &len);
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buf(
            len / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (!GetLogicalProcessorInformation(buf.data(), &len)) return 0;

        int cores = 0;
        for (auto& info : buf)
            if (info.Relationship == RelationProcessorCore) ++cores;
        return cores;
    }

    // ── Logical processor count ───────────────────────────────────────────────
    static int getLogicalProcessorCount() {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return (int)si.dwNumberOfProcessors;
    }

    // ── CPU rated speed from registry ─────────────────────────────────────────
    static int getCPURatedSpeedMHz() {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD speed = 0, size = sizeof(DWORD);
            RegQueryValueExA(hKey, "~MHz", NULL, NULL, (LPBYTE)&speed, &size);
            RegCloseKey(hKey);
            return (int)speed;
        }
        return 0;
    }

    // ── Full CPU query ────────────────────────────────────────────────────────
    static CPUInfo queryCPU() {
        CPUInfo info;
        info.brandString       = getCPUBrand();
        info.physicalCores     = getPhysicalCoreCount();
        info.logicalProcessors = getLogicalProcessorCount();
        info.ratedSpeedMHz     = getCPURatedSpeedMHz();
        return info;
    }

    // ── Real-time CPU usage % (call periodically — needs delta) ───────────────
    static double getCPUUsagePercent() {
        static ULONGLONG prevIdle = 0, prevKernel = 0, prevUser = 0;
        static bool first = true;

        FILETIME ftIdle, ftKernel, ftUser;
        GetSystemTimes(&ftIdle, &ftKernel, &ftUser);

        auto toULL = [](const FILETIME& ft) -> ULONGLONG {
            return ((ULONGLONG)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
        };
        ULONGLONG idle   = toULL(ftIdle);
        ULONGLONG kernel = toULL(ftKernel);
        ULONGLONG user   = toULL(ftUser);

        if (first) {
            prevIdle = idle; prevKernel = kernel; prevUser = user;
            first = false;
            Sleep(100);
            return getCPUUsagePercent(); // recurse once for first reading
        }

        ULONGLONG dIdle   = idle   - prevIdle;
        ULONGLONG dKernel = kernel - prevKernel;
        ULONGLONG dUser   = user   - prevUser;
        prevIdle = idle; prevKernel = kernel; prevUser = user;

        ULONGLONG total = dKernel + dUser;
        if (total == 0) return 0.0;
        return (1.0 - (double)dIdle / total) * 100.0;
    }

    // ── GPU info via DXGI ─────────────────────────────────────────────────────
    static GPUInfo queryGPU() {
        GPUInfo info;
        info.name = "Unknown GPU";
        info.dedicatedVRAM_MB = 0;
        info.sharedMemory_MB  = 0;

        IDXGIFactory* pFactory = nullptr;
        HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
        if (FAILED(hr) || !pFactory) return info;

        // Enumerate adapters; pick the first non-software one with most VRAM
        IDXGIAdapter* pBest = nullptr;
        size_t bestVRAM = 0;

        for (UINT i = 0; ; ++i) {
            IDXGIAdapter* pAdapter = nullptr;
            if (pFactory->EnumAdapters(i, &pAdapter) == DXGI_ERROR_NOT_FOUND) break;
            DXGI_ADAPTER_DESC desc;
            pAdapter->GetDesc(&desc);
            if (desc.DedicatedVideoMemory > bestVRAM) {
                if (pBest) pBest->Release();
                pBest = pAdapter;
                bestVRAM = desc.DedicatedVideoMemory;
            } else {
                pAdapter->Release();
            }
        }

        if (pBest) {
            DXGI_ADAPTER_DESC desc;
            pBest->GetDesc(&desc);
            char name[256] = {0};
            wcstombs(name, desc.Description, 255);
            info.name = name;
            info.dedicatedVRAM_MB = desc.DedicatedVideoMemory / (1024 * 1024);
            info.sharedMemory_MB  = desc.SharedSystemMemory   / (1024 * 1024);
            pBest->Release();
        }
        pFactory->Release();
        return info;
    }

    // ── Memory info via GlobalMemoryStatusEx ──────────────────────────────────
    static MemInfo queryMemory() {
        MEMORYSTATUSEX ms;
        ms.dwLength = sizeof(ms);
        GlobalMemoryStatusEx(&ms);

        MemInfo info;
        info.totalPhysical_MB = (uint64_t)(ms.ullTotalPhys / (1024 * 1024));
        info.availPhysical_MB = (uint64_t)(ms.ullAvailPhys / (1024 * 1024));
        info.usedPhysical_MB  = info.totalPhysical_MB - info.availPhysical_MB;
        info.usagePercent     = (int)ms.dwMemoryLoad;
        return info;
    }

    // ── Battery info via GetSystemPowerStatus ─────────────────────────────────
    static BatteryInfo queryBattery() {
        SYSTEM_POWER_STATUS sps;
        GetSystemPowerStatus(&sps);

        BatteryInfo info;
        info.hasBattery   = !(sps.BatteryFlag & 128);  // bit 128 = no battery
        info.chargePercent = (sps.BatteryLifePercent <= 100)
                             ? (int)sps.BatteryLifePercent : -1;
        info.isCharging   = (sps.BatteryFlag & 8) != 0; // bit 8 = charging
        info.acPluggedIn  = (sps.ACLineStatus == 1);
        return info;
    }

    // ── Process list via Toolhelp32 + GetProcessMemoryInfo ────────────────────
    static std::vector<ProcessEntry> queryProcesses(int maxCount = 20) {
        std::vector<ProcessEntry> list;

        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return list;

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        if (!Process32First(snap, &pe)) { CloseHandle(snap); return list; }

        do {
            ProcessEntry entry;
            entry.pid  = pe.th32ProcessID;
            entry.name = pe.szExeFile;
            entry.memoryMB = 0.0;

            // Try to read memory usage
            HANDLE hProc = OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
            if (hProc) {
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc))) {
                    entry.memoryMB = pmc.WorkingSetSize / (1024.0 * 1024.0);
                }
                CloseHandle(hProc);
            }
            list.push_back(entry);
        } while (Process32Next(snap, &pe));

        CloseHandle(snap);

        // Sort by memory desc, keep top N
        std::sort(list.begin(), list.end(),
                  [](const ProcessEntry& a, const ProcessEntry& b){
                      return a.memoryMB > b.memoryMB; });
        if ((int)list.size() > maxCount)
            list.resize(maxCount);

        return list;
    }

    // ── OS version string ─────────────────────────────────────────────────────
    static std::string getOSVersion() {
        HKEY hKey;
        std::string product = "Windows";
        std::string build   = "";
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char buf[256] = {0};
            DWORD sz = sizeof(buf);
            if (RegQueryValueExA(hKey, "ProductName", NULL, NULL,
                (LPBYTE)buf, &sz) == ERROR_SUCCESS)
                product = buf;
            sz = sizeof(buf);
            if (RegQueryValueExA(hKey, "DisplayVersion", NULL, NULL,
                (LPBYTE)buf, &sz) == ERROR_SUCCESS)
                build = buf;
            RegCloseKey(hKey);
        }
        return product + (build.empty() ? "" : " " + build);
    }

    // ── Computer name ─────────────────────────────────────────────────────────
    static std::string getComputerName() {
        char buf[MAX_COMPUTERNAME_LENGTH + 1] = {0};
        DWORD sz = sizeof(buf);
        ::GetComputerNameA(buf, &sz);
        return std::string(buf);
    }
};

#endif // SYSTEM_QUERY_H
