// SPDX-License-Identifier: MIT
#include "gui/ConfigScreens.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdio>

#include "nlohmann/json.hpp"

#include "config/GeneralOptions.h"
#include "config/ModConfigFile.h"
#include "modules/BaseModule.h"
#include "modules/EmptyModule.h"
#include "modules/HelpModule.h"
#include "modules/LeftModules1.h"
#include "modules/Modules2.h"
#include "render/Gui2d.h"
#include "utils/DebugOverlay.h"
#include "utils/Lang.h"

namespace better_f3::gui {

namespace {

std::string tr(std::string const& k) { return Lang::get().get(k); }
Component   trc(std::string const& k) { return Component::literal(tr(k)); }

std::string hexColor(std::uint32_t v, bool alpha) {
    char b[16];
    if (alpha) std::snprintf(b, sizeof b, "#%08X", v);
    else std::snprintf(b, sizeof b, "#%06X", v & 0xFFFFFF);
    return b;
}

std::optional<std::uint32_t> parseColor(std::string s, bool alpha) {
    if (!s.empty() && s[0] == '#') s = s.substr(1);
    if (s.size() != (alpha ? 8u : 6u)) return std::nullopt;
    std::uint32_t v = 0;
    auto [p, ec]    = std::from_chars(s.data(), s.data() + s.size(), v, 16);
    if (ec != std::errc() || p != s.data() + s.size()) return std::nullopt;
    return v;
}

std::optional<double> parseDouble(std::string const& s) {
    double d     = 0;
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), d);
    if (s.empty() || ec != std::errc() || p != s.data() + s.size() || !std::isfinite(d)) return std::nullopt;
    return d;
}

std::optional<long long> parseInt(std::string const& s) {
    long long v  = 0;
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc() || p != s.data() + s.size() || s.empty()) return std::nullopt;
    return v;
}

std::string fmtDouble(double d) {
    char b[32];
    std::snprintf(b, sizeof b, "%g", d);
    std::string s = b;
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos) s += ".0";
    return s;
}

void drawTooltip(Gui2d& g, float x, float y, std::vector<std::string> const& lines) {
    if (lines.empty()) return;
    float w = 0;
    for (auto const& l : lines) w = std::max(w, g.width(l));
    float const h  = static_cast<float>(lines.size()) * 10.f - 2.f;
    float       tx = x + 12.f, ty = y - 12.f;
    if (tx + w + 4.f > g.width()) tx = std::max(2.f, x - 16.f - w);
    if (ty + h + 4.f > g.height()) ty = g.height() - h - 4.f;
    ty = std::max(ty, 4.f);
    g.fill(tx - 3, ty - 4, tx + w + 3, ty + h + 4, 0xF0100010);
    g.fill(tx - 3, ty - 3, tx + w + 3, ty - 2, 0x505000FF);
    g.fill(tx - 3, ty + h + 2, tx + w + 3, ty + h + 3, 0x5028007F);
    g.fill(tx - 3, ty - 2, tx - 2, ty + h + 2, 0x505000FF);
    g.fill(tx + w + 2, ty - 2, tx + w + 3, ty + h + 2, 0x505000FF);
    for (size_t i = 0; i < lines.size(); ++i) g.text(Component::literal(lines[i]), tx, ty + static_cast<float>(i) * 10.f, 0xFFFFFFFF, true);
}

std::vector<std::string> wrap(Gui2d& g, std::string const& text, float maxW) {
    std::vector<std::string> out;
    std::string              line;
    size_t                   i = 0;
    while (i < text.size()) {
        size_t len = 1;
        unsigned char const c = static_cast<unsigned char>(text[i]);
        if (c >= 0xF0) len = 4;
        else if (c >= 0xE0) len = 3;
        else if (c >= 0xC0) len = 2;
        std::string const ch = text.substr(i, len);
        if (g.width(line + ch) > maxW && !line.empty()) {
            auto sp = line.rfind(' ');
            if (len == 1 && sp != std::string::npos && sp > 0) {
                out.push_back(line.substr(0, sp));
                line = line.substr(sp + 1);
            } else {
                out.push_back(line);
                line.clear();
            }
        }
        line += ch;
        i += len;
    }
    if (!line.empty()) out.push_back(line);
    return out;
}

class ConfigListScreen : public Screen {
public:
    enum class Type { Bool, Double, Int, Color, String, Choice };
    struct Entry {
        Type        type;
        std::string label;
        std::string tooltip;
        bool        boolValue = false, boolDefault = false;
        std::string text, textDefault;
        double      min = 0, max = 0;
        bool        alpha = false;
        std::vector<std::string> choices;
        int                      choice = 0, choiceDefault = 0;
        std::function<void(Entry const&)> save;
        std::function<bool(std::string const&)> check;
        Widget*    control = nullptr;
        Button*    reset   = nullptr;
        float      y       = 0;

        [[nodiscard]] bool valid() const {
            switch (type) {
            case Type::Double: {
                auto d = parseDouble(text);
                return d && *d >= min && *d <= max;
            }
            case Type::Int: return parseInt(text).has_value();
            case Type::Color: return parseColor(text, alpha).has_value();
            case Type::String: return !check || check(text);
            default: return true;
            }
        }
        [[nodiscard]] bool isDefault() const {
            switch (type) {
            case Type::Bool: return boolValue == boolDefault;
            case Type::Choice: return choice == choiceDefault;
            default: return text == textDefault;
            }
        }
        [[nodiscard]] double        doubleValue() const { return parseDouble(text).value_or(0); }
        [[nodiscard]] long long     intValue() const { return parseInt(text).value_or(0); }
        [[nodiscard]] std::uint32_t colorValue() const { return parseColor(text, alpha).value_or(0); }
    };

    ConfigListScreen(std::string id, Component title, std::function<void()> onSaved) : Screen(std::move(title)), mId(std::move(id)), mOnSaved(std::move(onSaved)) {}

    std::vector<Entry> entries;

    Entry& addBool(std::string label, std::string tip, bool value, bool def, std::function<void(Entry const&)> save) {
        Entry e{Type::Bool, std::move(label), std::move(tip)};
        e.boolValue = value, e.boolDefault = def, e.save = std::move(save);
        return entries.emplace_back(std::move(e));
    }
    Entry& addDouble(std::string label, std::string tip, double value, double def, double mn, double mx, std::function<void(Entry const&)> save) {
        Entry e{Type::Double, std::move(label), std::move(tip)};
        e.text = fmtDouble(value), e.textDefault = fmtDouble(def), e.min = mn, e.max = mx, e.save = std::move(save);
        return entries.emplace_back(std::move(e));
    }
    Entry& addInt(std::string label, std::string tip, long long value, long long def, std::function<void(Entry const&)> save) {
        Entry e{Type::Int, std::move(label), std::move(tip)};
        e.text = std::to_string(value), e.textDefault = std::to_string(def), e.save = std::move(save);
        return entries.emplace_back(std::move(e));
    }
    Entry& addColor(std::string label, std::string tip, std::uint32_t value, std::uint32_t def, bool alpha, std::function<void(Entry const&)> save) {
        Entry e{Type::Color, std::move(label), std::move(tip)};
        e.text = hexColor(value, alpha), e.textDefault = hexColor(def, alpha), e.alpha = alpha, e.save = std::move(save);
        return entries.emplace_back(std::move(e));
    }
    Entry& addString(std::string label, std::string tip, std::string value, std::string def, std::function<void(Entry const&)> save) {
        Entry e{Type::String, std::move(label), std::move(tip)};
        e.text = std::move(value), e.textDefault = std::move(def), e.save = std::move(save);
        return entries.emplace_back(std::move(e));
    }
    Entry& addChoice(std::string label, std::string tip, std::vector<std::string> choices, int value, std::function<void(Entry const&)> save) {
        Entry e{Type::Choice, std::move(label), std::move(tip)};
        e.choices = std::move(choices), e.choice = value, e.choiceDefault = value, e.save = std::move(save);
        return entries.emplace_back(std::move(e));
    }

    void init(float w, float h) override {
        float const rowW  = std::min(w - 20.f, 380.f);
        mLeft             = (w - rowW) / 2.f;
        mRight            = mLeft + rowW;
        mTop              = 32.f;
        mBottom           = h - 32.f;
        float const ctrlW = 150.f, resetW = 44.f;
        for (auto& e : entries) {
            Rect const rc{mRight - resetW - 4.f - ctrlW, 0, ctrlW, 20};
            if (e.type == Type::Bool || e.type == Type::Choice) {
                auto* b   = add<Button>(rc, Component(), nullptr);
                Entry* ep = &e;
                b->onPress = [this, ep] {
                    if (ep->type == Type::Bool) ep->boolValue = !ep->boolValue;
                    else mOpenChoice = static_cast<int>(ep - entries.data());
                    refresh();
                };
                e.control = b;
            } else {
                auto* f     = add<TextField>(Rect{rc.x + (e.type == Type::Color ? 22.f : 0.f), 0, rc.w - (e.type == Type::Color ? 22.f : 0.f), 20}, e.text);
                Entry* ep   = &e;
                f->onChange = [this, ep](std::string& v) {
                    ep->text = v;
                    refresh();
                };
                e.control = f;
            }
            Entry* ep = &e;
            e.reset   = add<Button>(Rect{mRight - resetW, 0, resetW, 20}, trc("betterf3.gui.reset"), [this, ep] {
                ep->boolValue = ep->boolDefault;
                ep->choice    = ep->choiceDefault;
                ep->text      = ep->textDefault;
                if (auto* f = dynamic_cast<TextField*>(ep->control)) f->value = ep->text;
                refresh();
            });
        }
        mCancel = add<Button>(Rect{w / 2.f - 154.f, h - 26.f, 150, 20}, trc("betterf3.gui.cancel"), [] { GuiManager::get().back(); });
        mSave   = add<Button>(Rect{w / 2.f + 4.f, h - 26.f, 150, 20}, trc("betterf3.gui.save"), [this] { saveAndClose(); });
        layout();
        refresh();
    }

    void render(Gui2d& g, float mx, float my) override {
        drawTitle(g, 12.f);
        g.fill(mLeft - 4.f, mTop, mRight + 4.f, mBottom, 0x60000000);
        std::vector<std::string> tooltip;
        for (auto const& e : entries) {
            if (!e.control || !e.control->visible) continue;
            std::uint32_t const col = e.valid() ? kWhite : 0xFFFF5555;
            Component const     lab = Component::literal(e.label);
            g.text(lab, mLeft + 2.f, e.y + 6.f, col, true);
            if (e.type == Type::Color) {
                std::uint32_t const c = e.colorValue();
                float const         x = e.control->r.x - 22.f;
                g.fill(x, e.y, x + 20.f, e.y + 20.f, 0xFFA0A0A0);
                g.fill(x + 1, e.y + 1, x + 19.f, e.y + 19.f, e.alpha ? c : (0xFF000000 | c));
            }
            if (Rect{mLeft, e.y, e.control->r.x - mLeft - 24.f, 20}.contains(mx, my) && !e.tooltip.empty()) tooltip = wrap(g, e.tooltip, 200.f);
        }
        Screen::render(g, mx, my);
        if (mOpenChoice >= 0 && mOpenChoice < static_cast<int>(entries.size())) {
            auto const& e = entries[mOpenChoice];
            float const x = e.control->r.x, y = e.control->r.y + 20.f, w = e.control->r.w;
            for (size_t i = 0; i < e.choices.size(); ++i) {
                Rect const rc{x, y + static_cast<float>(i) * 12.f, w, 12};
                g.fill(rc.x, rc.y, rc.x + rc.w, rc.y + rc.h, rc.contains(mx, my) ? 0xFF404040 : 0xFF000000);
                g.text(Component::literal(e.choices[i]), rc.x + 4.f, rc.y + 2.f, static_cast<int>(i) == e.choice ? 0xFFFFFF55 : kWhite, true);
            }
        }
        drawTooltip(g, mx, my, tooltip);
    }

    bool mouseClicked(float x, float y, int button) override {
        if (mOpenChoice >= 0) {
            auto&       e  = entries[mOpenChoice];
            float const cx = e.control->r.x, cy = e.control->r.y + 20.f, w = e.control->r.w;
            for (size_t i = 0; i < e.choices.size(); ++i) {
                if (Rect{cx, cy + static_cast<float>(i) * 12.f, w, 12}.contains(x, y)) e.choice = static_cast<int>(i);
            }
            mOpenChoice = -1;
            refresh();
            return true;
        }
        return Screen::mouseClicked(x, y, button);
    }

    bool mouseScrolled(float, float, float delta) override {
        mScroll = std::clamp(mScroll - delta * 24.f, 0.f, maxScroll());
        layout();
        return true;
    }

    [[nodiscard]] std::string id() const override { return mId; }

    [[nodiscard]] std::string debugJson() const override {
        auto arr = nlohmann::ordered_json::array();
        for (auto const& e : entries) {
            std::string v = e.type == Type::Bool ? (e.boolValue ? "true" : "false") : e.type == Type::Choice ? (e.choice >= 0 && e.choice < static_cast<int>(e.choices.size()) ? e.choices[e.choice] : "") : e.text;
            arr.push_back({{"label", e.label}, {"value", v}, {"valid", e.valid()}, {"default", e.isDefault()}, {"y", e.y}, {"visible", e.control && e.control->visible},
                           {"cx", e.control ? e.control->r.x + e.control->r.w / 2.f : 0.f}, {"rx", e.reset ? e.reset->r.x + e.reset->r.w / 2.f : 0.f}});
        }
        return nlohmann::ordered_json{{"entries", arr}, {"scroll", mScroll}, {"openChoice", mOpenChoice}, {"saveActive", mSave && mSave->active}}.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace);
    }

private:
    [[nodiscard]] float maxScroll() const { return std::max(0.f, static_cast<float>(entries.size()) * 24.f + 8.f - (mBottom - mTop)); }

    void layout() {
        float y = mTop + 4.f - mScroll;
        for (auto& e : entries) {
            e.y             = y;
            bool const show = y >= mTop && y + 20.f <= mBottom;
            if (e.control) {
                e.control->r.y     = y;
                e.control->visible = show;
            }
            if (e.reset) {
                e.reset->r.y     = y;
                e.reset->visible = show;
            }
            y += 24.f;
        }
    }

    void refresh() {
        bool ok = true;
        for (auto& e : entries) {
            if (auto* b = dynamic_cast<Button*>(e.control)) {
                if (e.type == Type::Bool) b->text = Component::literal(std::string(kSection) + (e.boolValue ? "a" : "c") + tr(e.boolValue ? "betterf3.gui.yes" : "betterf3.gui.no"));
                else b->text = Component::literal(e.choice >= 0 && e.choice < static_cast<int>(e.choices.size()) ? e.choices[e.choice] : "");
            }
            if (auto* f = dynamic_cast<TextField*>(e.control)) f->invalid = !e.valid();
            if (e.reset) e.reset->active = !e.isDefault();
            ok = ok && e.valid();
        }
        if (mSave) mSave->active = ok;
    }

    void saveAndClose() {
        for (auto const& e : entries) {
            if (!e.valid()) return;
        }
        for (auto const& e : entries) {
            if (e.save) e.save(e);
        }
        if (mOnSaved) mOnSaved();
        GuiManager::get().back();
    }

    std::string           mId;
    std::function<void()> mOnSaved;
    float                 mLeft = 0, mRight = 0, mTop = 0, mBottom = 0, mScroll = 0;
    int                   mOpenChoice = -1;
    Button*               mCancel     = nullptr;
    Button*               mSave       = nullptr;
};

std::unique_ptr<Screen> makeGeneralOptionsScreen() {
    auto s = std::make_unique<ConfigListScreen>("general", trc("config.betterf3.title"), [] { ModConfigFile::save(); });
    using E = ConfigListScreen::Entry;
    auto boolEntry = [&](char const* key, bool& ref, bool def) {
        s->addBool(tr(std::string("config.betterf3.") + key), tr(std::string("config.betterf3.") + key + ".tooltip"), ref, def, [&ref](E const& e) { ref = e.boolValue; });
    };
    boolEntry("disable", GeneralOptions::disableMod, false);
    boolEntry("auto_start", GeneralOptions::autoF3, false);
    boolEntry("space_modules", GeneralOptions::spaceEveryModule, false);
    boolEntry("performance_optimizations", GeneralOptions::enablePerformanceOptimizations, true);
    boolEntry("shadow_text", GeneralOptions::shadowText, true);
    boolEntry("animations", GeneralOptions::enableAnimations, true);
    s->addDouble(tr("config.betterf3.animationSpeed"), tr("config.betterf3.animationSpeed.tooltip"), GeneralOptions::animationSpeed, 1, 1, 3, [](E const& e) { GeneralOptions::animationSpeed = e.doubleValue(); });
    s->addDouble(tr("config.betterf3.fontScale"), tr("config.betterf3.fontScale.tooltip"), GeneralOptions::fontScale, 1, 0.1, 2, [](E const& e) { GeneralOptions::fontScale = e.doubleValue(); });
    s->addColor(tr("config.betterf3.color.background"), tr("config.betterf3.color.background.tooltip"), GeneralOptions::backgroundColor, 0x6F505050, true, [](E const& e) { GeneralOptions::backgroundColor = e.colorValue(); });
    boolEntry("debug_crosshair", GeneralOptions::hideDebugCrosshair, false);
    boolEntry("sidebar", GeneralOptions::hideSidebar, true);
    boolEntry("bossbar", GeneralOptions::hideBossbar, true);
    boolEntry("always_enable_profiler", GeneralOptions::alwaysEnableProfiler, false);
    s->addBool(tr("config.betterf3.always_enable_tps_graph"), tr("config.betterf3.always_enable_tps_graph.tooltip"), GeneralOptions::alwaysEnableTPS, false, [](E const& e) {
        if (e.boolValue && GeneralOptions::alwaysEnablePing) {
            GeneralOptions::alwaysEnablePing = false;
            GeneralOptions::alwaysEnableTPS  = true;
        } else {
            GeneralOptions::alwaysEnableTPS = e.boolValue;
        }
    });
    s->addBool(tr("config.betterf3.always_enable_ping_graph"), tr("config.betterf3.always_enable_ping_graph.tooltip"), GeneralOptions::alwaysEnablePing, false, [](E const& e) {
        if (e.boolValue && GeneralOptions::alwaysEnableTPS) {
            GeneralOptions::alwaysEnableTPS  = false;
            GeneralOptions::alwaysEnablePing = true;
        } else {
            GeneralOptions::alwaysEnablePing = e.boolValue;
        }
    });
    auto& key = s->addString(tr("config.betterf3.config_screen_key"), tr("config.betterf3.config_screen_key.tooltip"), std::string(1, GeneralOptions::configScreenKey), "M", [](E const& e) {
        GeneralOptions::configScreenKey = static_cast<char>(std::toupper(static_cast<unsigned char>(e.text[0])));
    });
    key.check = [](std::string const& v) { return v.size() == 1 && std::isalnum(static_cast<unsigned char>(v[0])); };
    return s;
}

class ModulesScreen : public Screen {
public:
    explicit ModulesScreen(PositionEnum side) : Screen(trc("config.betterf3.title.modules")), mSide(side) {
        mEntries = side == PositionEnum::LEFT ? BaseModule::modules : BaseModule::modulesRight;
    }

    void init(float w, float h) override {
        mListTop    = 32.f;
        mListBottom = h - 64.f;
        mRowLeft    = w / 2.f - kRowWidth / 2.f + 2.f;
        mDelete     = add<Button>(Rect{w / 2.f - 154.f, h - 50.f, 100, 20}, trc("config.betterf3.modules.delete_button"), [this] {
            if (mSelected >= 0 && mSelected < static_cast<int>(mEntries.size())) {
                mEntries.erase(mEntries.begin() + mSelected);
                mSelected = -1;
                mScroll   = std::min(mScroll, maxScroll());
                updateButtons();
            }
        });
        mEdit       = add<Button>(Rect{w / 2.f - 50.f, h - 50.f, 100, 20}, trc("config.betterf3.modules.edit_button"), [this] {
            if (mSelected >= 0 && mSelected < static_cast<int>(mEntries.size())) GuiManager::get().open(makeEditScreen(mEntries[mSelected]));
        });
        add<Button>(Rect{w / 2.f + 4.f + 50.f, h - 50.f, 100, 20}, trc("config.betterf3.modules.add_button"), [this] { GuiManager::get().open(makeAddScreen()); });
        add<Button>(Rect{w / 2.f - 154.f, h - 30.f + 4.f, 308, 20}, trc("config.betterf3.modules.done_button"), [this] { onClose(); });
        updateButtons();
    }

    void render(Gui2d& g, float mx, float my) override {
        drawTitle(g, 20.f);
        g.fill(0, mListTop, width, mListBottom, 0x60000000);
        for (size_t i = 0; i < mEntries.size(); ++i) {
            float const y = rowTop(i);
            if (y - 2.f < mListTop || y + kItemHeight - 2.f > mListBottom) continue;
            auto const& m     = *mEntries[i];
            float const x     = mRowLeft;
            bool const  hover = mx >= x - 2.f && mx < x + kRowWidth - 4.f && my >= y - 2.f && my < y + kItemHeight - 2.f && my >= mListTop && my < mListBottom;
            if (static_cast<int>(i) == mSelected) {
                g.fill(x - 2.f, y - 2.f, x + kRowWidth - 4.f, y + kItemHeight - 2.f, 0xFFFFFFFF);
                g.fill(x - 1.f, y - 1.f, x + kRowWidth - 5.f, y + kItemHeight - 3.f, 0xFF000000);
            }
            g.text(Component::literal(m.toString()), x + 35.f, y + 1.f, kWhite, true);
            g.text(exampleText(m), x + 43.f, y + 13.f, kWhite, true);
            if (hover) {
                g.fill(x, y, x + 32.f, y + 32.f, 0xA0909090);
                float const v = mx - x, w = my - y;
                if (i > 0) arrow(g, x, y, true, v < 16 && w < 16);
                if (i + 1 < mEntries.size()) arrow(g, x, y, false, v < 16 && w > 16);
            }
        }
        Screen::render(g, mx, my);
    }

    bool mouseClicked(float x, float y, int button) override {
        if (Screen::mouseClicked(x, y, button)) return true;
        if (button != 1 || y < mListTop || y >= mListBottom) return false;
        for (size_t i = 0; i < mEntries.size(); ++i) {
            float const top = rowTop(i);
            if (y < top - 2.f || y >= top + kItemHeight - 2.f || x < mRowLeft - 2.f || x >= mRowLeft + kRowWidth - 4.f) continue;
            float const d = x - mRowLeft, e = y - top;
            if (d <= 32.f) {
                if (d < 16.f && e < 16.f && i > 0) {
                    std::swap(mEntries[i], mEntries[i - 1]);
                    mSelected = static_cast<int>(i - 1);
                    updateButtons();
                    return true;
                }
                if (d < 16.f && e > 16.f && i + 1 < mEntries.size()) {
                    std::swap(mEntries[i], mEntries[i + 1]);
                    mSelected = static_cast<int>(i + 1);
                    updateButtons();
                    return true;
                }
            }
            mSelected = static_cast<int>(i);
            updateButtons();
            return true;
        }
        return false;
    }

    bool mouseScrolled(float, float, float delta) override {
        mScroll = std::clamp(mScroll - delta * kItemHeight / 2.f, 0.f, maxScroll());
        return true;
    }

    void onClose() override {
        (mSide == PositionEnum::LEFT ? BaseModule::modules : BaseModule::modulesRight) = mEntries;
        ModConfigFile::save();
        GuiManager::get().back();
    }

    [[nodiscard]] bool        showsDebugBehind() const override { return true; }
    [[nodiscard]] std::string id() const override { return mSide == PositionEnum::LEFT ? "modules_left" : "modules_right"; }

    [[nodiscard]] std::string debugJson() const override {
        auto arr = nlohmann::ordered_json::array();
        for (size_t i = 0; i < mEntries.size(); ++i) arr.push_back({{"id", mEntries[i]->id}, {"name", mEntries[i]->toString()}, {"top", rowTop(i)}});
        return nlohmann::ordered_json{{"entries", arr}, {"selected", mSelected}, {"rowLeft", mRowLeft}, {"scroll", mScroll}}.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace);
    }

    void addModule(BaseModule::Ptr m) { mEntries.push_back(std::move(m)); }

private:
    static constexpr float kRowWidth   = 305.f;
    static constexpr float kItemHeight = 36.f;

    [[nodiscard]] float rowTop(size_t i) const { return mListTop + 4.f + static_cast<float>(i) * kItemHeight - mScroll; }
    [[nodiscard]] float maxScroll() const { return std::max(0.f, static_cast<float>(mEntries.size()) * kItemHeight + 8.f - (mListBottom - mListTop)); }

    void updateButtons() {
        bool const sel = mSelected >= 0 && mSelected < static_cast<int>(mEntries.size());
        if (mEdit) mEdit->active = sel;
        if (mDelete) mDelete->active = sel;
    }

    static void arrow(Gui2d& g, float x, float y, bool up, bool hi) {
        std::uint32_t const c = hi ? 0xFFFFFFFF : 0xFFA0A0A0;
        for (int i = 0; i < 6; ++i) {
            float const half = static_cast<float>(i);
            float const yy   = up ? y + 5.f + half : y + 26.f - half;
            g.fill(x + 8.f - half, yy, x + 9.f + half, yy + 1.f, c);
        }
    }

    static Component exampleText(BaseModule const& m) {
        if (auto const* c = dynamic_cast<CoordsModule const*>(&m)) {
            Component t = utils::styledText("X", c->colorX);
            t.append(utils::styledText("Y", c->colorY)).append(utils::styledText("Z", c->colorZ)).append(utils::styledText(": ", c->nameColor));
            t.append(utils::styledText("100 ", c->colorX)).append(utils::styledText("200 ", c->colorY)).append(utils::styledText("300", c->colorZ));
            return t;
        }
        if (auto const* f = dynamic_cast<FpsModule const*>(&m)) {
            Component t = utils::styledText("60 fps  ", f->colorHigh);
            t.append(utils::styledText("40 fps  ", f->colorMed)).append(utils::styledText("10 fps", f->colorLow));
            return t;
        }
        Component t = utils::styledText("Name: ", m.nameColor);
        t.append(utils::styledText("Value", m.valueColor));
        return t;
    }

    std::unique_ptr<Screen> makeEditScreen(BaseModule::Ptr const& module);
    std::unique_ptr<Screen> makeAddScreen();

    PositionEnum                  mSide;
    std::vector<BaseModule::Ptr>  mEntries;
    int                           mSelected = -1;
    float                         mScroll = 0, mListTop = 0, mListBottom = 0, mRowLeft = 0;
    Button*                       mEdit   = nullptr;
    Button*                       mDelete = nullptr;
};

std::unique_ptr<Screen> ModulesScreen::makeEditScreen(BaseModule::Ptr const& module) {
    auto  s = std::make_unique<ConfigListScreen>("edit_" + module->id, Component::literal(module->toString()), [module] {
        module->markDirty();
        ModConfigFile::save();
    });
    using E = ConfigListScreen::Entry;
    BaseModule* m = module.get();
    s->addBool(tr("config.betterf3.module.enable"), tr("config.betterf3.module.enable.tooltip"), m->enabled, true, [m](E const& e) { m->enable(e.boolValue); });
    auto color = [&](char const* key, TextColor& ref, TextColor def) {
        s->addColor(tr(std::string("config.betterf3.color.") + key), tr(std::string("config.betterf3.color.") + key + ".tooltip"), ref, def, false, [&ref](E const& e) { ref = e.colorValue() & 0xFFFFFF; });
    };
    if (auto* c = dynamic_cast<CoordsModule*>(m)) {
        color("x", c->colorX, c->defaultColorX);
        color("y", c->colorY, c->defaultColorY);
        color("z", c->colorZ, c->defaultColorZ);
    }
    if (auto* f = dynamic_cast<FpsModule*>(m)) {
        color("fps.high", f->colorHigh, f->defaultColorHigh);
        color("fps.medium", f->colorMed, f->defaultColorMed);
        color("fps.low", f->colorLow, f->defaultColorLow);
    }
    if (auto* em = dynamic_cast<EmptyModule*>(m)) {
        s->addInt(tr("config.betterf3.empty_lines"), tr("config.betterf3.empty_lines.tooltip"), em->emptyLines, em->defaultEmptyLines, [em](E const& e) {
            em->emptyLines = static_cast<int>(std::clamp<long long>(e.intValue(), 1, 20));
        });
    }
    if (auto* c = dynamic_cast<ChunksModule*>(m)) {
        color("chunks.enabled", c->enabledColor, c->defaultEnabledColor);
        color("chunks.disabled", c->disabledColor, c->defaultDisabledColor);
        color("chunks.total", c->totalColor, c->defaultTotalColor);
    }
    if (auto* en = dynamic_cast<EntityModule*>(m)) color("entities.total", en->totalColor, en->defaultTotalColor);
    if (auto* h = dynamic_cast<HelpModule*>(m)) {
        color("help.enabled", h->enabledColor, h->defaultEnabledColor);
        color("help.disabled", h->disabledColor, h->defaultDisabledColor);
    }
    if (auto* so = dynamic_cast<SoundModule*>(m)) color("sound.maximum", so->maximumColor, so->defaultMaximumColor);
    if (auto* sy = dynamic_cast<SystemModule*>(m)) {
        s->addBool(tr("config.betterf3.memory_color_toggle"), tr("config.betterf3.memory_color_toggle.tooltip"), sy->memoryColorToggle, sy->defaultMemoryColorToggle, [sy](E const& e) { sy->memoryColorToggle = e.boolValue; });
        s->addString(tr("config.betterf3.time_format"), tr("config.betterf3.time_format.tooltip"), sy->timeFormat, sy->defaultTimeFormat, [sy](E const& e) { sy->timeFormat = e.text; });
    }
    color("name", m->nameColor, m->defaultNameColor);
    color("value", m->valueColor, m->defaultValueColor);
    if (m->lines().size() > 1) {
        for (auto& line : m->lines()) {
            if (line->id().empty() || line->id() == "nothing") continue;
            std::string const key  = "text.betterf3.line." + line->id();
            std::string       name = tr(key);
            if (name.empty() || name == key) name = utils::enumToString(line->id());
            DebugLine* lp = line.get();
            s->addBool(name, tr("config.betterf3.disable_line.tooltip"), lp->enabled, true, [lp](E const& e) { lp->enabled = e.boolValue; });
        }
    }
    return s;
}

std::unique_ptr<Screen> ModulesScreen::makeAddScreen() {
    auto distinct = BaseModule::distinctModules();
    std::vector<std::string> names;
    std::vector<std::string> ids;
    int                      emptyIndex = 0;
    for (auto const& m : distinct) {
        if (m->id == "empty") emptyIndex = static_cast<int>(names.size());
        names.push_back(m->toString());
        ids.push_back(m->id);
    }
    auto s = std::make_unique<ConfigListScreen>("add_module", trc("config.betterf3.title.modules"), nullptr);
    using E = ConfigListScreen::Entry;
    s->addChoice(tr("config.betterf3.add_button.module_name"), "", names, emptyIndex, [this, ids](E const& e) {
        if (e.choice < 0 || e.choice >= static_cast<int>(ids.size())) return;
        auto m = BaseModule::create(ids[e.choice]);
        if (m) addModule(std::move(m));
    });
    return s;
}

class ModConfigScreen : public Screen {
public:
    ModConfigScreen() : Screen(trc("config.betterf3.title.config")) {}

    void init(float w, float h) override {
        add<Button>(Rect{w / 2.f - 130.f, h / 4.f, 120, 20}, trc("config.betterf3.order_left_button"), [] { GuiManager::get().open(makeModulesScreen(PositionEnum::LEFT)); });
        add<Button>(Rect{w / 2.f + 10.f, h / 4.f, 120, 20}, trc("config.betterf3.order_right_button"), [] { GuiManager::get().open(makeModulesScreen(PositionEnum::RIGHT)); });
        add<Button>(Rect{w / 2.f - 130.f, h / 4.f - 24.f, 260, 20}, trc("config.betterf3.general_settings"), [] { GuiManager::get().open(makeGeneralOptionsScreen()); });
        add<Button>(Rect{w / 2.f - 130.f, h - 50.f, 260, 20}, trc("config.betterf3.modules.done_button"), [] { GuiManager::get().back(); });
    }

    void render(Gui2d& g, float mx, float my) override {
        Screen::render(g, mx, my);
        drawTitle(g, 20.f);
    }

    [[nodiscard]] std::string id() const override { return "config"; }
};

}

std::unique_ptr<Screen> makeModConfigScreen() { return std::make_unique<ModConfigScreen>(); }
std::unique_ptr<Screen> makeModulesScreen(PositionEnum side) { return std::make_unique<ModulesScreen>(side); }

void openConfigScreen() {
    GuiManager::get().open(makeModConfigScreen());
    if (!DebugOverlay::isOverlayVisible) DebugOverlay::toggleOverlay();
}

}
