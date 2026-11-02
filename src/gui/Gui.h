// SPDX-License-Identifier: MIT
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "utils/Component.h"

namespace better_f3 {
class Gui2d;
}

namespace better_f3::gui {

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    [[nodiscard]] bool contains(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

class Widget {
public:
    virtual ~Widget() = default;
    virtual void render(Gui2d& g, float mx, float my) = 0;
    virtual bool mouseClicked(float, float, int) { return false; }
    virtual bool keyPressed(int) { return false; }
    virtual bool charTyped(std::string const&) { return false; }
    [[nodiscard]] virtual bool focused() const { return false; }
    virtual void               setFocused(bool) {}
    [[nodiscard]] virtual std::string kind() const = 0;
    [[nodiscard]] virtual std::string label() const { return ""; }

    Rect r;
    bool active  = true;
    bool visible = true;
};

class Button : public Widget {
public:
    Button(Rect rect, Component text, std::function<void()> onPress);
    void render(Gui2d& g, float mx, float my) override;
    bool mouseClicked(float x, float y, int button) override;
    [[nodiscard]] std::string kind() const override { return "button"; }
    [[nodiscard]] std::string label() const override { return text.getString(); }

    Component             text;
    std::function<void()> onPress;
};

class TextField : public Widget {
public:
    TextField(Rect rect, std::string value);
    void render(Gui2d& g, float mx, float my) override;
    bool mouseClicked(float x, float y, int button) override;
    bool keyPressed(int code) override;
    bool charTyped(std::string const& s) override;
    [[nodiscard]] bool        focused() const override { return mFocused; }
    void                      setFocused(bool f) override { mFocused = f; }
    [[nodiscard]] std::string kind() const override { return "text"; }
    [[nodiscard]] std::string label() const override { return value; }

    std::string                        value;
    bool                               invalid = false;
    std::function<void(std::string&)> onChange;

private:
    bool mFocused = false;
};

class Screen {
public:
    explicit Screen(Component title) : title(std::move(title)) {}
    virtual ~Screen() = default;

    virtual void init(float width, float height) = 0;
    virtual void render(Gui2d& g, float mx, float my);
    virtual bool mouseClicked(float x, float y, int button);
    virtual bool mouseScrolled(float, float, float) { return false; }
    virtual bool keyPressed(int code);
    virtual bool charTyped(std::string const& s);
    virtual void onClose();
    [[nodiscard]] virtual bool showsDebugBehind() const { return false; }
    [[nodiscard]] virtual std::string id() const = 0;
    [[nodiscard]] virtual std::string debugJson() const { return "{}"; }

    Component                            title;
    std::vector<std::unique_ptr<Widget>> widgets;
    float                                width = 0, height = 0;

protected:
    template <class T, class... A>
    T* add(A&&... a) {
        auto p   = std::make_unique<T>(std::forward<A>(a)...);
        T*   raw = p.get();
        widgets.push_back(std::move(p));
        return raw;
    }
    void drawTitle(Gui2d& g, float y = 20.f);
};

class GuiManager {
public:
    static GuiManager& get();

    bool install();
    void uninstall();

    void open(std::unique_ptr<Screen> s);
    void replace(std::unique_ptr<Screen> s);
    void back();
    void closeAll();
    [[nodiscard]] bool    isOpen() const { return !mStack.empty(); }
    [[nodiscard]] Screen* top() const { return mStack.empty() ? nullptr : mStack.back().get(); }
    [[nodiscard]] size_t  depth() const { return mStack.size(); }
    [[nodiscard]] bool    showsDebugBehind() const { return !top() || top()->showsDebugBehind(); }

    void render(Gui2d& g);

    void onMouseMove(float x, float y) { mMouseX = x, mMouseY = y; }
    bool onMouseButton(float x, float y, int button);
    bool onScroll(float delta);
    bool onKey(int code);
    bool onText(std::string const& s);

    struct RawMouse {
        int moves = 0, buttons = 0, wheels = 0, others = 0;
        int lastAction = -1, lastData = 0, lastX = 0, lastY = 0;
    };
    [[nodiscard]] RawMouse const& rawMouse() const { return mRaw; }
    RawMouse                      mRaw;

    [[nodiscard]] float mouseX() const { return mMouseX; }
    [[nodiscard]] float mouseY() const { return mMouseY; }

private:
    std::vector<std::unique_ptr<Screen>> mStack;
    std::vector<std::unique_ptr<Screen>> mGraveyard;
    float                                mMouseX = -1, mMouseY = -1;
    float                                mLastW = 0, mLastH = 0;
};

inline constexpr std::uint32_t kWhite = 0xFFFFFFFF;
inline constexpr std::uint32_t kGray  = 0xFFA0A0A0;

}
