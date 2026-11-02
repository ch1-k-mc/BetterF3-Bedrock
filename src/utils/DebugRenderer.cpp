// SPDX-License-Identifier: MIT
#include "utils/DebugRenderer.h"

#include <unordered_map>

#include "mc/client/game/ClientInstance.h"
#include "mc/client/player/LocalPlayer.h"

#include "config/GeneralOptions.h"
#include "gui/Gui.h"
#include "modules/BaseModule.h"
#include "render/Gui2d.h"
#include "utils/DebugOverlay.h"
#include "utils/ServerSampler.h"

namespace better_f3::DebugRenderer {

namespace {

constexpr std::int64_t kStaticRefreshIntervalMs = 250;
constexpr int          kLineHeight              = 9;
constexpr std::uint32_t kTextColor              = 0xFFE0E0E0;

std::unordered_map<BaseModule const*, std::int64_t> gLastUpdateTimes;
std::vector<Component>                              gLastLeft;
std::vector<Component>                              gLastRight;
std::vector<std::string>                            gExtraLeft;
std::vector<std::string>                            gExtraRight;

bool shouldUpdateModuleNow(BaseModule const& module, std::int64_t now) {
    if (module.updatesEveryFrame()) return true;
    if (module.needsUpdate()) return true;
    auto it = gLastUpdateTimes.find(&module);
    return it == gLastUpdateTimes.end() || now - it->second >= kStaticRefreshIntervalMs;
}

bool screenAllowsDrawing(ClientInstance& client) {
    if (!gui::GuiManager::get().showsDebugBehind()) return false;
    auto const top = client.getTopScreenName();
    return top == "hud_screen" || top == "chat_screen";
}

float fs() { return static_cast<float>(GeneralOptions::fontScale); }

}

std::vector<std::string>& extraLeft() { return gExtraLeft; }
std::vector<std::string>& extraRight() { return gExtraRight; }
std::vector<Component> const& lastLeft() { return gLastLeft; }
std::vector<Component> const& lastRight() { return gLastRight; }

std::vector<Component> newText(ClientInstance& client, bool left, std::vector<std::string> const& extra) {
    auto const&            list = left ? BaseModule::modules : BaseModule::modulesRight;
    std::vector<Component> out;
    out.reserve(std::max<size_t>(16, list.size() * 6));
    if (client.getLocalPlayer() == nullptr || !screenAllowsDrawing(client)) return out;

    bool const         reducedDebug = false;
    std::int64_t const now          = utils::millis();
    for (auto const& module : list) {
        if (!module->enabled) continue;
        if (shouldUpdateModuleNow(*module, now)) {
            (void)extra;
            module->update(client);
            module->refreshDirtyFromState();
            gLastUpdateTimes[module.get()] = now;
        }
        auto const& lines = module->cachedLinesFormatted(reducedDebug);
        out.insert(out.end(), lines.begin(), lines.end());
        if (GeneralOptions::spaceEveryModule) out.push_back(Component::literal(""));
    }
    return out;
}

void drawBackground(Gui2d& g, float screenWidth, PositionEnum pos, std::vector<Component> const& list) {
    float const s = fs();
    for (size_t i = 0; i < list.size(); ++i) {
        int const width = static_cast<int>(g.width(list[i]));
        if (width == 0) continue;
        int const y = 2 + kLineHeight * static_cast<int>(i);
        int       x1, x2;
        int       windowWidth;
        if (pos == PositionEnum::RIGHT) {
            windowWidth = static_cast<int>(screenWidth / GeneralOptions::fontScale) - 2 - width;
            if (GeneralOptions::enableAnimations) windowWidth += utils::xPos;
            x1 = windowWidth - 1;
            x2 = windowWidth + width + 1;
        } else {
            windowWidth = 2;
            if (GeneralOptions::enableAnimations) windowWidth -= utils::xPos;
            x1 = windowWidth - 1;
            x2 = width + 1 + windowWidth;
        }
        int const y1 = y - 1;
        int const y2 = y + kLineHeight - 1;
        g.fill(static_cast<float>(x1) * s, static_cast<float>(y1) * s, static_cast<float>(x2) * s, static_cast<float>(y2) * s, GeneralOptions::backgroundColor);
    }
}

void drawRightText(Gui2d& g, float screenWidth, std::vector<Component>& list) {
    drawBackground(g, screenWidth, PositionEnum::RIGHT, list);
    float const s = fs();
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].getString().empty()) continue;
        int const width       = static_cast<int>(g.width(list[i]));
        int       windowWidth = static_cast<int>(screenWidth / GeneralOptions::fontScale) - 2 - width;
        if (GeneralOptions::enableAnimations) windowWidth += utils::xPos;
        int const y = 2 + kLineHeight * static_cast<int>(i);
        g.text(list[i], static_cast<float>(windowWidth) * s, static_cast<float>(y) * s, kTextColor, GeneralOptions::shadowText, s);
    }
}

void drawLeftText(Gui2d& g, float screenWidth, std::vector<Component>& list) {
    drawBackground(g, screenWidth, PositionEnum::LEFT, list);
    float const s = fs();
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].getString().empty()) continue;
        int const y        = 2 + kLineHeight * static_cast<int>(i);
        int       xPosLeft = 2;
        if (GeneralOptions::enableAnimations) xPosLeft -= utils::xPos;
        g.text(list[i], static_cast<float>(xPosLeft) * s, static_cast<float>(y) * s, kTextColor, GeneralOptions::shadowText, s);
    }
}

void animate() {
    if (GeneralOptions::disableMod || !GeneralOptions::enableAnimations) return;
    std::int64_t const time = utils::millis();
    using utils::closingAnimation;
    using utils::xPos;
    if (time - utils::lastAnimationUpdate >= 10 && (xPos != 0 || closingAnimation)) {
        int i = ((utils::START_X_POS / 2 + xPos) / 10) - 9;
        if (xPos != 0 && !closingAnimation) {
            xPos  = static_cast<int>(xPos / GeneralOptions::animationSpeed);
            xPos -= i;
        }
        if (i == 0) i = 1;
        if (closingAnimation) {
            xPos += i;
            xPos  = static_cast<int>(xPos * GeneralOptions::animationSpeed);
            if (xPos >= 300) {
                closingAnimation               = false;
                DebugOverlay::isOverlayVisible = false;
            }
        }
        utils::lastAnimationUpdate = time;
    }
}

void render(Gui2d& g, ClientInstance& client) {
    if (GeneralOptions::disableMod || !DebugOverlay::isOverlayVisible) {
        gLastLeft.clear();
        gLastRight.clear();
        return;
    }
    animate();
    if (!DebugOverlay::isOverlayVisible) return;
    ServerSampler::request();
    float const width = g.width();
    gLastLeft         = newText(client, true, gExtraLeft);
    drawLeftText(g, width, gLastLeft);
    gLastRight = newText(client, false, gExtraRight);
    drawRightText(g, width, gLastRight);
}

}
