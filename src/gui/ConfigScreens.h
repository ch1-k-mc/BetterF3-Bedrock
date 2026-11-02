// SPDX-License-Identifier: MIT
#pragma once

#include <memory>

#include "gui/Gui.h"
#include "utils/Utils.h"

namespace better_f3::gui {

void openConfigScreen();

[[nodiscard]] std::unique_ptr<Screen> makeModConfigScreen();
[[nodiscard]] std::unique_ptr<Screen> makeModulesScreen(PositionEnum side);

}
