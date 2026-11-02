// SPDX-License-Identifier: MIT
#include "hooks/DebugHooks.h"

#include <set>
#include <vector>

#include "ll/api/event/EventBus.h"
#include "ll/api/event/client/ClientJoinLevelEvent.h"
#include "ll/api/event/input/KeyInputEvent.h"

#include "gui/Gui.h"
#include "ll/api/event/render/UIRenderEvent.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/screens/ScreenView.h"

#include "graphs/DebugCharts.h"
#include "hooks/HudHide.h"
#include "net/NetStats.h"
#include "input/F3Combos.h"
#include "render/Gui2d.h"
#include "utils/DebugOverlay.h"
#include "utils/DebugRenderer.h"
#include "utils/Lang.h"

namespace better_f3::hooks {

namespace {

constexpr int kKeyF3 = 114;

std::vector<ll::event::ListenerPtr> gListeners;
bool                                gF3Down  = false;
bool                                gF3Used  = false;
ComboHandler                        gCombo;
ReleaseHandler                      gRelease;
std::set<int>                       gDown;
unsigned long long                  gDraws   = 0;

bool inGameHud() {
    auto ci = ll::service::getClientInstance();
    return ci && ci->getLocalPlayer() != nullptr && ci->getTopScreenName() == "hud_screen";
}

void onKey(ll::event::KeyInputEvent& e) {
    int const  code    = e.keyCode();
    bool const repeat  = e.isDown() && gDown.contains(code);
    if (e.isDown()) gDown.insert(code);
    else gDown.erase(code);
    if (e.isCancelled()) {
        if (code == kKeyF3 && !e.isDown()) {
            if (gF3Down && gRelease) gRelease(true);
            gF3Down = false;
        }
        return;
    }
    if (code == kKeyF3) {
        if (e.isDown()) {
            if (!gF3Down) {
                gF3Down = true;
                gF3Used = false;
            }
        } else {
            if (gF3Down && !gF3Used && inGameHud()) DebugOverlay::toggleOverlay();
            if (gF3Down && gRelease) gRelease(gF3Used);
            gF3Down = false;
        }
        return;
    }
    if (!gF3Down && e.isDown() && !repeat && code >= '0' && code <= '9' && inGameHud() && DebugCharts::onDigit(code - '0')) {
        e.cancel();
        return;
    }
    if (gF3Down && e.isDown() && !repeat) {
        gF3Used = true;
        if (gCombo && inGameHud() && gCombo(code)) e.cancel();
    }
}

}

void setComboHandler(ComboHandler handler) { gCombo = std::move(handler); }
void setReleaseHandler(ReleaseHandler handler) { gRelease = std::move(handler); }
bool isKeyDown(int keyCode) { return gDown.contains(keyCode); }
bool isShiftDown() { return isKeyDown(16) || isKeyDown(160) || isKeyDown(161); }
bool isCtrlDown() { return isKeyDown(17) || isKeyDown(162) || isKeyDown(163); }
bool isF3Down() { return gF3Down; }
void noteSwallowedKeyUp(int keyCode) {
    gDown.erase(keyCode);
    if (keyCode == kKeyF3) {
        if (gF3Down && gRelease) gRelease(true);
        gF3Down = false;
    }
}
unsigned long long drawCount() { return gDraws; }

void start() {
    auto& bus = ll::event::EventBus::getInstance();
    gListeners.push_back(bus.emplaceListener<ll::event::KeyInputEvent>(onKey));
    gListeners.push_back(bus.emplaceListener<ll::event::AfterUIRenderEvent>([](ll::event::AfterUIRenderEvent& e) {
        if (e.screenView().getScreenName() != "hud_screen") return;
        auto ci = ll::service::getClientInstance();
        if (!ci) return;
        Lang::get().refresh();
        Gui2d g(e.uiRenderContext(), *ci);
        DebugRenderer::render(g, *ci);
        NetStats::onFrame();
        DebugCharts::onFrame(g);
        HudHide::drawDebugCrosshair(g);
        F3Combos::onFrame(g);
        gui::GuiManager::get().render(g);
        ++gDraws;
    }));
    gListeners.push_back(bus.emplaceListener<ll::event::ClientJoinLevelEvent>([](ll::event::ClientJoinLevelEvent&) {
        DebugOverlay::reset();
    }));
}

void stop() {
    auto& bus = ll::event::EventBus::getInstance();
    for (auto& l : gListeners) bus.removeListener(l);
    gListeners.clear();
}

}
