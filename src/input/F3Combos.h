// SPDX-License-Identifier: MIT
#pragma once

#include <string>

namespace better_f3 {
class Gui2d;
}

namespace better_f3::F3Combos {

bool install();
void uninstall();

void onFrame(Gui2d& g);

[[nodiscard]] bool advancedTooltips();
[[nodiscard]] bool chunkBorders();
[[nodiscard]] bool hitboxes();
[[nodiscard]] bool pauseOnLostFocus();
[[nodiscard]] bool switcherOpen();
[[nodiscard]] bool profiling();
[[nodiscard]] std::string const& lastClipboard();

bool press(int keyCode, bool shift = false, bool ctrl = false);
void releaseF3();

}
