// SPDX-License-Identifier: MIT
#pragma once

namespace better_f3 {
class Gui2d;
}

namespace better_f3::HudHide {

void start();
void stop();

void drawDebugCrosshair(Gui2d& g);

[[nodiscard]] int hiddenCount();

}
