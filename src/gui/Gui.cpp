// SPDX-License-Identifier: MIT
#include "gui/Gui.h"

#include <algorithm>

#include "ll/api/event/EventBus.h"
#include "ll/api/event/input/KeyInputEvent.h"
#include "ll/api/event/input/MouseInputEvent.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/game/MinecraftGame.h"
#include "mc/client/gui/GuiData.h"
#include "mc/deps/input/MouseAction.h"
#include "mc/deps/input/win/HIDControllerGameCoreDesktop.h"

#include "hooks/DebugHooks.h"
#include "render/Gui2d.h"
#include "utils/Utils.h"

namespace better_f3::gui {

namespace {

constexpr int VK_BACK = 8, VK_RETURN = 13, VK_ESCAPE = 27, VK_TAB = 9;

std::vector<ll::event::ListenerPtr> gListeners;
bool                                gAllowGrab = true;
bool                                gHooked    = false;

bool hudOnTop() {
    auto ci = ll::service::getClientInstance();
    return ci && ci->getTopScreenName() == "hud_screen";
}

float guiScale() {
    auto ci = ll::service::getClientInstance();
    if (!ci) return 1.f;
    float const s = ci->getGuiData()->mGuiScale;
    return s > 0 ? s : 1.f;
}

bool utils_blink() { return (utils::millis() / 500) % 2 == 0; }

void eraseLastUtf8(std::string& s) {
    if (s.empty()) return;
    size_t i = s.size() - 1;
    while (i > 0 && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) --i;
    s.erase(i);
}

}

Button::Button(Rect rect, Component t, std::function<void()> press) : text(std::move(t)), onPress(std::move(press)) { r = rect; }

void Button::render(Gui2d& g, float mx, float my) {
    if (!visible) return;
    bool const hover = active && r.contains(mx, my);
    g.fill(r.x, r.y, r.x + r.w, r.y + r.h, hover ? 0xFFFFFFFF : 0xFF000000);
    std::uint32_t const face = !active ? 0xFF2E2E2E : 0xFF6F6F6F;
    g.fill(r.x + 1, r.y + 1, r.x + r.w - 1, r.y + r.h - 1, face);
    if (active) {
        g.fill(r.x + 1, r.y + 1, r.x + r.w - 1, r.y + 2, 0xFFA7A7A7);
        g.fill(r.x + 1, r.y + r.h - 2, r.x + r.w - 1, r.y + r.h - 1, 0xFF4E4E4E);
    }
    float const tw = g.width(text);
    g.text(text, r.x + (r.w - tw) / 2.f, r.y + (r.h - 8.f) / 2.f, active ? kWhite : kGray, true);
}

bool Button::mouseClicked(float x, float y, int button) {
    if (!visible || !active || button != 1 || !r.contains(x, y)) return false;
    if (onPress) onPress();
    return true;
}

TextField::TextField(Rect rect, std::string v) : value(std::move(v)) { r = rect; }

void TextField::render(Gui2d& g, float, float) {
    if (!visible) return;
    g.fill(r.x, r.y, r.x + r.w, r.y + r.h, mFocused ? 0xFFFFFFFF : 0xFFA0A0A0);
    g.fill(r.x + 1, r.y + 1, r.x + r.w - 1, r.y + r.h - 1, 0xFF000000);
    std::string shown = value;
    float const maxW  = r.w - 8.f;
    while (!shown.empty() && g.width(shown) > maxW) shown.erase(shown.begin());
    std::uint32_t const col = invalid ? 0xFFFF5555 : 0xFFE0E0E0;
    g.text(Component::literal(shown + (mFocused && (utils_blink()) ? "_" : "")), r.x + 4.f, r.y + (r.h - 8.f) / 2.f, col, true);
}

bool TextField::mouseClicked(float x, float y, int button) {
    mFocused = visible && active && button == 1 && r.contains(x, y);
    return mFocused;
}

bool TextField::keyPressed(int code) {
    if (!mFocused) return false;
    if (code == VK_BACK) {
        eraseLastUtf8(value);
        if (onChange) onChange(value);
        return true;
    }
    if (code == VK_RETURN || code == VK_TAB) {
        mFocused = false;
        return true;
    }
    return code != VK_ESCAPE;
}

bool TextField::charTyped(std::string const& s) {
    if (!mFocused) return false;
    for (char c : s) {
        if (static_cast<unsigned char>(c) < 0x20) continue;
        value.push_back(c);
    }
    if (onChange) onChange(value);
    return true;
}

void Screen::render(Gui2d& g, float mx, float my) {
    for (auto& w : widgets) w->render(g, mx, my);
}

bool Screen::mouseClicked(float x, float y, int button) {
    bool handled = false;
    for (auto& w : widgets) {
        if (w->kind() == "text") {
            if (w->mouseClicked(x, y, button)) handled = true;
            continue;
        }
        if (!handled && w->mouseClicked(x, y, button)) {
            handled = true;
            break;
        }
    }
    return handled;
}

bool Screen::keyPressed(int code) {
    for (auto& w : widgets) {
        if (w->focused() && w->keyPressed(code)) return true;
    }
    if (code == VK_ESCAPE) {
        onClose();
        return true;
    }
    return false;
}

bool Screen::charTyped(std::string const& s) {
    for (auto& w : widgets) {
        if (w->focused() && w->charTyped(s)) return true;
    }
    return false;
}

void Screen::onClose() { GuiManager::get().back(); }

void Screen::drawTitle(Gui2d& g, float y) {
    float const tw = g.width(title);
    g.text(title, (width - tw) / 2.f, y, kWhite, true);
}

GuiManager& GuiManager::get() {
    static GuiManager m;
    return m;
}

void GuiManager::open(std::unique_ptr<Screen> s) {
    bool const first = mStack.empty();
    mLastW = mLastH = 0;
    mStack.push_back(std::move(s));
    if (first) {
        gAllowGrab = false;
        if (auto ci = ll::service::getClientInstance()) ci->releaseMouse();
    }
}

void GuiManager::replace(std::unique_ptr<Screen> s) {
    if (!mStack.empty()) {
        mGraveyard.push_back(std::move(mStack.back()));
        mStack.pop_back();
    }
    open(std::move(s));
}

void GuiManager::back() {
    if (mStack.empty()) return;
    mGraveyard.push_back(std::move(mStack.back()));
    mStack.pop_back();
    mLastW = mLastH = 0;
    if (mStack.empty()) closeAll();
}

void GuiManager::closeAll() {
    while (!mStack.empty()) {
        mGraveyard.push_back(std::move(mStack.back()));
        mStack.pop_back();
    }
    if (!gAllowGrab) {
        gAllowGrab = true;
        if (auto ci = ll::service::getClientInstance()) ci->grabMouse();
    }
}

void GuiManager::render(Gui2d& g) {
    mGraveyard.clear();
    Screen* s = top();
    if (!s) return;
    if (g.width() != mLastW || g.height() != mLastH) {
        mLastW    = g.width();
        mLastH    = g.height();
        s->width  = mLastW;
        s->height = mLastH;
        s->widgets.clear();
        s->init(mLastW, mLastH);
    }
    g.fill(0, 0, g.width(), g.height(), 0xA0101010);
    s->render(g, mMouseX, mMouseY);
}

bool GuiManager::onMouseButton(float x, float y, int button) {
    mMouseX = x, mMouseY = y;
    if (Screen* s = top()) s->mouseClicked(x, y, button);
    return isOpen();
}

bool GuiManager::onScroll(float delta) {
    if (Screen* s = top()) s->mouseScrolled(mMouseX, mMouseY, delta);
    return isOpen();
}

bool GuiManager::onKey(int code) {
    if (Screen* s = top()) s->keyPressed(code);
    return isOpen();
}

bool GuiManager::onText(std::string const& s) {
    if (Screen* t = top()) return t->charTyped(s);
    return false;
}

LL_TYPE_INSTANCE_HOOK(BetterF3ClientGrabMouseHook, ll::memory::HookPriority::Normal, ClientInstance, &ClientInstance::$grabMouse, void) {
    if (!gAllowGrab) return;
    origin();
}

LL_TYPE_INSTANCE_HOOK(BetterF3GameGrabMouseHook, ll::memory::HookPriority::Normal, MinecraftGame, &MinecraftGame::$grabMouse, void) {
    if (!gAllowGrab) return;
    origin();
}

LL_TYPE_INSTANCE_HOOK(
    BetterF3TextInputHook,
    ll::memory::HookPriority::Normal,
    HIDControllerGameCoreDesktop,
    &HIDControllerGameCoreDesktop::$onTextInput,
    void,
    ::std::string const& utf8Text
) {
    if (GuiManager::get().isOpen() && GuiManager::get().onText(utf8Text)) return;
    origin(utf8Text);
}

bool GuiManager::install() {
    auto& bus = ll::event::EventBus::getInstance();
    gListeners.push_back(bus.emplaceListener<ll::event::KeyInputEvent>(
        [this](ll::event::KeyInputEvent& e) {
            if (!isOpen() || !hudOnTop()) return;
            if (e.isDown()) onKey(e.keyCode());
            else hooks::noteSwallowedKeyUp(e.keyCode());
            e.cancel();
        },
        ll::event::EventPriority::High
    ));
    gListeners.push_back(bus.emplaceListener<ll::event::MouseInputEvent>(
        [this](ll::event::MouseInputEvent& e) {
            if (!isOpen() || !hudOnTop()) return;
            float const s      = guiScale();
            int const   action = static_cast<int>(e.actionButtonId());
            mRaw.lastAction = action, mRaw.lastData = static_cast<int>(e.buttonData()), mRaw.lastX = e.x(), mRaw.lastY = e.y();
            if (action == MouseAction::ActionMove) ++mRaw.moves;
            else if (action == MouseAction::ActionWheel) ++mRaw.wheels;
            else if (action == MouseAction::ActionLeft || action == MouseAction::ActionRight) ++mRaw.buttons;
            else ++mRaw.others;
            if (action == MouseAction::ActionMove) {
                onMouseMove(static_cast<float>(e.x()) / s, static_cast<float>(e.y()) / s);
            } else if (action == MouseAction::ActionWheel) {
                onScroll(static_cast<float>(e.buttonData()) > 0 ? 1.f : -1.f);
            } else if (action == MouseAction::ActionLeft || action == MouseAction::ActionRight) {
                if (e.buttonData() == MouseAction::DataDown) {
                    onMouseButton(mMouseX, mMouseY, action);
                }
            }
            e.cancel();
        },
        ll::event::EventPriority::High
    ));
    if (!gHooked) {
        bool ok = BetterF3ClientGrabMouseHook::hook() == 0;
        ok      = (BetterF3GameGrabMouseHook::hook() == 0) && ok;
        ok      = (BetterF3TextInputHook::hook() == 0) && ok;
        gHooked = true;
        return ok;
    }
    return true;
}

void GuiManager::uninstall() {
    auto& bus = ll::event::EventBus::getInstance();
    for (auto& l : gListeners) bus.removeListener(l);
    gListeners.clear();
    closeAll();
    if (gHooked) {
        BetterF3ClientGrabMouseHook::unhook();
        BetterF3GameGrabMouseHook::unhook();
        BetterF3TextInputHook::unhook();
        gHooked = false;
    }
}

}
