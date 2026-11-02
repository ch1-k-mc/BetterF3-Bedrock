// SPDX-License-Identifier: MIT
#include "config/GeneralOptions.h"

namespace better_f3::GeneralOptions {

void resetToDefaults() {
    disableMod                     = false;
    autoF3                         = false;
    spaceEveryModule               = false;
    shadowText                     = true;
    enableAnimations               = true;
    animationSpeed                 = 1;
    fontScale                      = 1;
    backgroundColor                = 0x6F505050;
    hideDebugCrosshair             = false;
    hideSidebar                    = true;
    hideBossbar                    = true;
    alwaysEnableProfiler           = false;
    alwaysEnableTPS                = false;
    alwaysEnablePing               = false;
    enablePerformanceOptimizations = true;
    configScreenKey                = 'M';
}

}
