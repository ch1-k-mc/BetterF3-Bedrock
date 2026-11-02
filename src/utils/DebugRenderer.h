// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/utils/DebugRenderer.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#pragma once

#include <string>
#include <vector>

#include "utils/Component.h"
#include "utils/Utils.h"

class ClientInstance;

namespace better_f3 {

class Gui2d;

namespace DebugRenderer {

std::vector<Component> newText(ClientInstance& client, bool left, std::vector<std::string> const& extra);

void drawBackground(Gui2d& g, float screenWidth, PositionEnum pos, std::vector<Component> const& list);
void drawLeftText(Gui2d& g, float screenWidth, std::vector<Component>& list);
void drawRightText(Gui2d& g, float screenWidth, std::vector<Component>& list);

void animate();

void render(Gui2d& g, ClientInstance& client);

std::vector<Component> const& lastLeft();
std::vector<Component> const& lastRight();

std::vector<std::string>& extraLeft();
std::vector<std::string>& extraRight();

}
}
