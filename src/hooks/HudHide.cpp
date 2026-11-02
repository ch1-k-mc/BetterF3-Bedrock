// SPDX-License-Identifier: MIT
#include "hooks/HudHide.h"

#include <cmath>
#include <deque>
#include <memory>
#include <vector>

#include "ll/api/event/EventBus.h"
#include "ll/api/event/render/UIRenderEvent.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/controls/UIControl.h"
#include "mc/client/gui/controls/VisualTree.h"

#include "gui/Gui.h"
#include "mc/client/gui/screens/ScreenView.h"
#include "mc/client/player/LocalPlayer.h"

#include "config/GeneralOptions.h"
#include "render/Gui2d.h"
#include "utils/DebugOverlay.h"

namespace better_f3::HudHide {

namespace {

struct Hidden {
    std::weak_ptr<UIControl> control;
    ui::VisibilityFlag       before;
};

std::vector<ll::event::ListenerPtr> gListeners;
std::vector<Hidden>                 gHidden;

std::shared_ptr<UIControl> findByName(std::shared_ptr<UIControl> const& root, std::string const& name) {
    std::deque<std::shared_ptr<UIControl>> q{root};
    int                                     guard = 0;
    while (!q.empty() && guard++ < 4000) {
        auto c = q.front();
        q.pop_front();
        if (!c) continue;
        if (c->mName.get() == name) return c;
        for (auto const& child : c->mChildren.get()) q.push_back(child);
    }
    return nullptr;
}

bool isHidden(UIControl const* c) {
    for (auto const& h : gHidden) {
        if (h.control.lock().get() == c) return true;
    }
    return false;
}

void hide(std::shared_ptr<UIControl> const& c) {
    if (!c) return;
    if (!isHidden(c.get())) gHidden.push_back({c, c->mVisible});
    c->mVisible = ui::VisibilityFlag::None;
}

void restore(std::string const& screen, std::vector<std::string> const& names, std::shared_ptr<UIControl> const& root) {
    for (auto const& n : names) {
        auto c = findByName(root, n);
        if (!c) continue;
        for (auto it = gHidden.begin(); it != gHidden.end(); ++it) {
            if (it->control.lock() == c) {
                c->mVisible = it->before;
                gHidden.erase(it);
                break;
            }
        }
    }
    (void)screen;
}

bool f3Shown() { return !GeneralOptions::disableMod && DebugOverlay::isOverlayVisible; }

void onBeforeRender(ll::event::BeforeUIRenderEvent& e) {
    auto const name = e.screenView().getScreenName();
    if (name != "hud_screen" && name != "hud_crosshair_screen") return;
    auto* tree = e.screenView().mVisualTree.get();
    if (!tree) return;
    std::shared_ptr<UIControl> root = tree->mRootControl;
    if (!root) return;
    if (name == "hud_screen") {
        if (f3Shown() && GeneralOptions::hideBossbar) hide(findByName(root, "boss_health_panel"));
        else restore(name, {"boss_health_panel"}, root);
        if (f3Shown() && GeneralOptions::hideSidebar) hide(findByName(root, "sidebar"));
        else restore(name, {"sidebar"}, root);
        bool const gui = gui::GuiManager::get().isOpen();
        for (char const* n : {"centered_gui_elements_at_bottom_middle", "chat_stack"}) {
            if (gui) hide(findByName(root, n));
            else restore(name, {n}, root);
        }
    } else {
        if ((f3Shown() && !GeneralOptions::hideDebugCrosshair) || gui::GuiManager::get().isOpen()) hide(findByName(root, "cursor"));
        else restore(name, {"cursor"}, root);
    }
    std::erase_if(gHidden, [](Hidden const& h) { return h.control.expired(); });
}

void line(Gui2d& g, float x0, float y0, float x1, float y1, std::uint32_t argb, float width) {
    float const dx    = x1 - x0;
    float const dy    = y1 - y0;
    int const   steps = std::max(1, static_cast<int>(std::ceil(std::max(std::abs(dx), std::abs(dy)) * 2.f)));
    for (int i = 0; i <= steps; ++i) {
        float const t = static_cast<float>(i) / static_cast<float>(steps);
        float const x = x0 + dx * t;
        float const y = y0 + dy * t;
        g.fill(x - width / 2.f, y - width / 2.f, x + width / 2.f, y + width / 2.f, argb);
    }
}

}

void drawDebugCrosshair(Gui2d& g) {
    if (!f3Shown() || GeneralOptions::hideDebugCrosshair || gui::GuiManager::get().isOpen()) return;
    auto ci = ll::service::getClientInstance();
    auto* p = ci ? ci->getLocalPlayer() : nullptr;
    if (!p) return;
    Vec2 const  rot   = p->getRotation();
    float const yaw   = rot.y * 3.14159265f / 180.f;
    float const pitch = rot.x * 3.14159265f / 180.f;
    Vec3 const right{-std::cos(yaw), 0.f, -std::sin(yaw)};
    Vec3 const fwd{-std::sin(yaw) * std::cos(pitch), -std::sin(pitch), std::cos(yaw) * std::cos(pitch)};
    Vec3 const up{right.y * fwd.z - right.z * fwd.y, right.z * fwd.x - right.x * fwd.z, right.x * fwd.y - right.y * fwd.x};
    float const cx  = g.width() / 2.f;
    float const cy  = g.height() / 2.f;
    float const len = 10.f;
    struct Axis {
        Vec3          v;
        std::uint32_t argb;
    };
    for (Axis const& a : {Axis{{1, 0, 0}, 0xFFFF0000}, Axis{{0, 1, 0}, 0xFF00FF00}, Axis{{0, 0, 1}, 0xFF0000FF}}) {
        float const sx = a.v.x * right.x + a.v.y * right.y + a.v.z * right.z;
        float const sy = -(a.v.x * up.x + a.v.y * up.y + a.v.z * up.z);
        line(g, cx, cy, cx + sx * len, cy + sy * len, a.argb, 1.f);
    }
}

int hiddenCount() { return static_cast<int>(gHidden.size()); }

void start() {
    auto& bus = ll::event::EventBus::getInstance();
    gListeners.push_back(bus.emplaceListener<ll::event::BeforeUIRenderEvent>(onBeforeRender));
}

void stop() {
    auto& bus = ll::event::EventBus::getInstance();
    for (auto& l : gListeners) bus.removeListener(l);
    gListeners.clear();
    for (auto& h : gHidden) {
        if (auto c = h.control.lock()) c->mVisible = h.before;
    }
    gHidden.clear();
}

}
