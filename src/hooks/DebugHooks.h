// SPDX-License-Identifier: MIT
#pragma once

#include <functional>

namespace better_f3::hooks {

void start();
void stop();

using ComboHandler = std::function<bool(int keyCode)>;
void setComboHandler(ComboHandler handler);
using ReleaseHandler = std::function<void(bool usedForCombo)>;
void setReleaseHandler(ReleaseHandler handler);
[[nodiscard]] bool isKeyDown(int keyCode);
[[nodiscard]] bool isShiftDown();
[[nodiscard]] bool isCtrlDown();

[[nodiscard]] bool isF3Down();
void noteSwallowedKeyUp(int keyCode);
[[nodiscard]] unsigned long long drawCount();

}
