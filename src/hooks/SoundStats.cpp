// SPDX-License-Identifier: MIT
#include "hooks/SoundStats.h"

#include <atomic>

#include <windows.h>

#include "ll/api/memory/Hook.h"
#include "ll/api/mod/NativeMod.h"

#include "utils/Utils.h"

namespace better_f3::SoundStats {

namespace {

using FnGetChannelsPlaying  = int(__cdecl*)(void* system, int* channels, int* realChannels);
using FnGetSoftwareChannels = int(__cdecl*)(void* system, int* num);
using FnGetChannel          = int(__cdecl*)(void* system, int id, void** channel);
using FnIsPlaying           = int(__cdecl*)(void* channel, int* isPlaying);
using FnGetCurrentSound     = int(__cdecl*)(void* channel, void** sound);
using FnGetMode             = int(__cdecl*)(void* sound, unsigned int* mode);
using FnSystemUpdate        = int(__fastcall*)(void* self);

constexpr unsigned int kCreateStream = 0x00000080;

std::atomic<void*>    gSystem{nullptr};
FnSystemUpdate        gOrigin  = nullptr;
void*                 gTarget  = nullptr;
FnGetChannelsPlaying  pGetChannelsPlaying  = nullptr;
FnGetSoftwareChannels pGetSoftwareChannels = nullptr;
FnGetChannel          pGetChannel          = nullptr;
FnIsPlaying           pIsPlaying           = nullptr;
FnGetCurrentSound     pGetCurrentSound     = nullptr;
FnGetMode             pGetMode             = nullptr;
Snapshot              gSnap;
std::int64_t          gLast = 0;

int __fastcall updateDetour(void* self) {
    gSystem.store(self, std::memory_order_relaxed);
    return gOrigin(self);
}

template <class T>
T proc(HMODULE m, char const* name) {
    return reinterpret_cast<T>(GetProcAddress(m, name));
}

}

bool install() {
    HMODULE fmod = GetModuleHandleW(L"fmod.dll");
    if (!fmod) return false;
    pGetChannelsPlaying  = proc<FnGetChannelsPlaying>(fmod, "FMOD5_System_GetChannelsPlaying");
    pGetSoftwareChannels = proc<FnGetSoftwareChannels>(fmod, "FMOD5_System_GetSoftwareChannels");
    pGetChannel          = proc<FnGetChannel>(fmod, "FMOD5_System_GetChannel");
    pIsPlaying           = proc<FnIsPlaying>(fmod, "FMOD5_Channel_IsPlaying");
    pGetCurrentSound     = proc<FnGetCurrentSound>(fmod, "FMOD5_Channel_GetCurrentSound");
    pGetMode             = proc<FnGetMode>(fmod, "FMOD5_Sound_GetMode");
    gTarget              = reinterpret_cast<void*>(GetProcAddress(fmod, "?update@System@FMOD@@QEAA?AW4FMOD_RESULT@@XZ"));
    if (!gTarget || !pGetChannelsPlaying || !pGetSoftwareChannels) return false;
    int const rc = ll::memory::hook(gTarget, reinterpret_cast<void*>(&updateDetour), reinterpret_cast<void**>(&gOrigin), ll::memory::HookPriority::Normal);
    if (rc != 0) gTarget = nullptr;
    return rc == 0;
}

void uninstall() {
    if (gTarget) ll::memory::unhook(gTarget, reinterpret_cast<void*>(&updateDetour));
    gTarget = nullptr;
    gSystem = nullptr;
}

Snapshot get() {
    void* sys = gSystem.load(std::memory_order_relaxed);
    if (!sys) return gSnap;
    std::int64_t const now = utils::millis();
    if (gSnap.valid && now - gLast < 250) return gSnap;
    gLast = now;
    Snapshot s;
    int      channels = 0, real = 0, soft = 0;
    if (pGetChannelsPlaying(sys, &channels, &real) != 0) return gSnap;
    if (pGetSoftwareChannels(sys, &soft) != 0) soft = 0;
    int streaming = 0;
    if (pGetChannel && pIsPlaying && pGetCurrentSound && pGetMode) {
        int const limit = soft > 0 ? soft : 256;
        for (int i = 0; i < limit; ++i) {
            void* ch = nullptr;
            if (pGetChannel(sys, i, &ch) != 0 || !ch) continue;
            int playing = 0;
            if (pIsPlaying(ch, &playing) != 0 || !playing) continue;
            void* sound = nullptr;
            if (pGetCurrentSound(ch, &sound) != 0 || !sound) continue;
            unsigned int mode = 0;
            if (pGetMode(sound, &mode) == 0 && (mode & kCreateStream)) ++streaming;
        }
    }
    s.streaming   = streaming;
    s.playing     = std::max(0, channels - streaming);
    s.maxChannels = soft;
    s.valid       = true;
    gSnap         = s;
    return gSnap;
}

}
