// SPDX-License-Identifier: MIT
#include "utils/SystemInfo.h"

#include <thread>
#include <vector>

#include <windows.h>

#include <dxgi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <psapi.h>

#include "ll/api/utils/SystemUtils.h"

#include "utils/Utils.h"

namespace better_f3::SystemInfo {

namespace {

std::string narrow(std::wstring const& w) {
    if (w.empty()) return {};
    int const n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

std::string trim(std::string s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\0')) s.pop_back();
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    return s;
}

std::string vendorName(UINT id) {
    switch (id) {
    case 0x10DE: return "NVIDIA Corporation";
    case 0x1002:
    case 0x1022: return "AMD";
    case 0x8086: return "Intel";
    case 0x1414: return "Microsoft";
    case 0x5143: return "Qualcomm";
    default: {
        char b[16];
        std::snprintf(b, sizeof b, "0x%04X", id);
        return b;
    }
    }
}

struct GpuCounter {
    PDH_HQUERY   query   = nullptr;
    PDH_HCOUNTER counter = nullptr;
    bool         failed  = false;
    std::int64_t last    = 0;
    std::optional<double> value;

    std::optional<double> sample() {
        if (failed) return std::nullopt;
        std::int64_t const now = utils::millis();
        if (!query) {
            if (PdhOpenQueryW(nullptr, 0, &query) != ERROR_SUCCESS) {
                failed = true;
                return std::nullopt;
            }
            std::wstring path = L"\\GPU Engine(pid_" + std::to_wstring(GetCurrentProcessId()) + L"_*engtype_3D)\\Utilization Percentage";
            if (PdhAddEnglishCounterW(query, path.c_str(), 0, &counter) != ERROR_SUCCESS) {
                failed = true;
                return std::nullopt;
            }
            PdhCollectQueryData(query);
            last = now;
            return std::nullopt;
        }
        if (now - last < 1000) return value;
        last = now;
        if (PdhCollectQueryData(query) != ERROR_SUCCESS) return value;
        DWORD bufSize = 0, count = 0;
        if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufSize, &count, nullptr) != PDH_MORE_DATA) return value;
        std::vector<std::byte> buf(bufSize);
        auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buf.data());
        if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufSize, &count, items) != ERROR_SUCCESS) return value;
        double sum = 0;
        for (DWORD i = 0; i < count; ++i) {
            if (items[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA || items[i].FmtValue.CStatus == PDH_CSTATUS_NEW_DATA) sum += items[i].FmtValue.doubleValue;
        }
        value = sum;
        return value;
    }
};

GpuCounter gGpuCounter;

}

std::string const& cpuInfo() {
    static std::string const info = [] {
        std::wstring name;
        HKEY         key = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &key) == ERROR_SUCCESS) {
            wchar_t buf[256]{};
            DWORD   size = sizeof buf;
            DWORD   type = 0;
            if (RegQueryValueExW(key, L"ProcessorNameString", nullptr, &type, reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS && type == REG_SZ) name = buf;
            RegCloseKey(key);
        }
        return std::to_string(std::thread::hardware_concurrency()) + "x " + trim(narrow(name));
    }();
    return info;
}

Gpu const& gpu() {
    static Gpu const g = [] {
        Gpu           out;
        IDXGIFactory1* factory = nullptr;
        if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory))) || !factory) return out;
        IDXGIAdapter1* adapter = nullptr;
        if (SUCCEEDED(factory->EnumAdapters1(0, &adapter)) && adapter) {
            DXGI_ADAPTER_DESC1 desc{};
            if (SUCCEEDED(adapter->GetDesc1(&desc))) {
                out.name   = trim(narrow(desc.Description));
                out.vendor = vendorName(desc.VendorId);
            }
            LARGE_INTEGER umd{};
            if (SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &umd))) {
                auto const v = static_cast<std::uint64_t>(umd.QuadPart);
                out.driver   = std::to_string(v >> 48) + "." + std::to_string((v >> 32) & 0xFFFF) + "." + std::to_string((v >> 16) & 0xFFFF) + "." + std::to_string(v & 0xFFFF);
            }
            adapter->Release();
        }
        factory->Release();
        return out;
    }();
    return g;
}

std::string graphicsApi() {
    if (GetModuleHandleW(L"d3d12.dll")) return "Direct3D 12";
    if (GetModuleHandleW(L"d3d11.dll")) return "Direct3D 11";
    return "";
}

std::optional<double> gpuUtilization() { return gGpuCounter.sample(); }

std::uint64_t processCommit() {
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof pmc)) return pmc.PrivateUsage;
    return 0;
}

bool postFocusMessage(bool lost) {
    struct Ctx {
        DWORD pid;
        HWND  found;
    } ctx{::GetCurrentProcessId(), nullptr};
    ::EnumWindows(
        [](HWND h, LPARAM l) -> BOOL {
            auto* c   = reinterpret_cast<Ctx*>(l);
            DWORD pid = 0;
            ::GetWindowThreadProcessId(h, &pid);
            if (pid != c->pid) return TRUE;
            wchar_t cls[64]{};
            ::GetClassNameW(h, cls, 64);
            if (std::wstring_view(cls) != L"Bedrock") return TRUE;
            c->found = h;
            return FALSE;
        },
        reinterpret_cast<LPARAM>(&ctx)
    );
    if (!ctx.found) return false;
    return ::PostMessageW(ctx.found, lost ? WM_KILLFOCUS : WM_SETFOCUS, 0, 0) != 0;
}

std::string osName() { return ll::sys_utils::getSystemName() + " " + ll::sys_utils::getSystemVersion().to_string(); }

}
