// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/config/GeneralOptions.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#pragma once

#include <cstdint>

namespace better_f3::GeneralOptions {

inline bool          disableMod                     = false;
inline bool          autoF3                         = false;
inline bool          spaceEveryModule               = false;
inline bool          shadowText                     = true;
inline bool          enableAnimations               = true;
inline double        animationSpeed                 = 1;
inline double        fontScale                      = 1;
inline std::uint32_t backgroundColor                = 0x6F505050;
inline bool          hideDebugCrosshair             = false;
inline bool          hideSidebar                    = true;
inline bool          hideBossbar                    = true;
inline bool          alwaysEnableProfiler           = false;
inline bool          alwaysEnableTPS                = false;
inline bool          alwaysEnablePing               = false;
inline bool          enablePerformanceOptimizations = true;
inline char          configScreenKey                = 'M';

void resetToDefaults();

}
