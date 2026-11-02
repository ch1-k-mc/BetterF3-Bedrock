// SPDX-License-Identifier: MIT
#include "command/BetterF3Command.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <sstream>

#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/client/ClientJoinLevelEvent.h"
#include "ll/api/event/client/ClientStartJoinLevelEvent.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/GuiData.h"
#include "mc/network/LoopbackPacketSender.h"
#include "mc/network/MinecraftPacketIds.h"
#include "mc/network/Packet.h"
#include "mc/network/packet/CommandRequestPacket.h"
#include "mc/network/packet/CommandRequestPacketPayload.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/server/commands/CommandRawText.h"

#include "config/GeneralOptions.h"
#include "config/ModConfigFile.h"
#include "modules/BaseModule.h"
#include "modules/Modules.h"
#include "net/NetStats.h"
#include "utils/DebugOverlay.h"
#include "utils/Lang.h"
#include "utils/TextColor.h"

struct BetterF3CommandArgs {
    CommandRawText args;
};

namespace better_f3::command {

namespace {

std::vector<std::string> gOut;
bool                     gToChat = true;

void say(std::string const& s) {
    gOut.push_back(s);
    if (!gToChat) return;
    if (auto ci = ll::service::getClientInstance()) ci->getGuiData()->displayClientMessage(s, std::nullopt, false);
}
std::string t(char const* key, std::vector<std::string> const& args = {}) { return utils::i18n(std::string("betterf3.command.") + key, args); }

void ok(std::string const& s) { say(std::string(kSection) + "e[BetterF3] " + std::string(kSection) + "r" + s); }
void fail(std::string const& s) { say(std::string(kSection) + "c[BetterF3] " + s); }

std::vector<std::string> split(std::string const& s) {
    std::vector<std::string> out;
    std::istringstream       in(s);
    for (std::string w; in >> w;) out.push_back(w);
    return out;
}

std::optional<bool> parseBool(std::string v) {
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (v == "true" || v == "on" || v == "1" || v == "yes") return true;
    if (v == "false" || v == "off" || v == "0" || v == "no") return false;
    return std::nullopt;
}

std::optional<double> parseDouble(std::string const& v) {
    try {
        size_t idx = 0;
        double d   = std::stod(v, &idx);
        if (idx != v.size() || !std::isfinite(d)) return std::nullopt;
        return d;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::int64_t> parseColor(std::string v) {
    int base = 10;
    if (!v.empty() && v[0] == '#') {
        v    = v.substr(1);
        base = 16;
    } else if (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) {
        v    = v.substr(2);
        base = 16;
    }
    std::int64_t r   = 0;
    auto [p, ec]     = std::from_chars(v.data(), v.data() + v.size(), r, base);
    if (ec != std::errc() || p != v.data() + v.size()) return std::nullopt;
    return r;
}

std::string hex(std::uint32_t v, int digits) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "#%0*X", digits, v);
    return buf;
}

struct GeneralKey {
    char const* key;
    enum { Bool, Double, Color, Key } type;
    void* ptr;
    double min = 0, max = 0;
};

std::vector<GeneralKey> const& generalKeys() {
    static std::vector<GeneralKey> const keys{
        {"disable_mod",               GeneralKey::Bool,   &GeneralOptions::disableMod                    },
        {"auto_start",                GeneralKey::Bool,   &GeneralOptions::autoF3                        },
        {"space_modules",             GeneralKey::Bool,   &GeneralOptions::spaceEveryModule              },
        {"shadow_text",               GeneralKey::Bool,   &GeneralOptions::shadowText                    },
        {"animations",                GeneralKey::Bool,   &GeneralOptions::enableAnimations              },
        {"animationSpeed",            GeneralKey::Double, &GeneralOptions::animationSpeed,           1, 3},
        {"fontScale",                 GeneralKey::Double, &GeneralOptions::fontScale,              0.1, 2},
        {"background_color",          GeneralKey::Color,  &GeneralOptions::backgroundColor               },
        {"hide_debug_crosshair",      GeneralKey::Bool,   &GeneralOptions::hideDebugCrosshair            },
        {"hide_sidebar",              GeneralKey::Bool,   &GeneralOptions::hideSidebar                   },
        {"hide_bossbar",              GeneralKey::Bool,   &GeneralOptions::hideBossbar                   },
        {"always_show_profiler",      GeneralKey::Bool,   &GeneralOptions::alwaysEnableProfiler          },
        {"always_show_tps",           GeneralKey::Bool,   &GeneralOptions::alwaysEnableTPS               },
        {"always_show_ping",          GeneralKey::Bool,   &GeneralOptions::alwaysEnablePing              },
        {"performance_optimizations", GeneralKey::Bool,   &GeneralOptions::enablePerformanceOptimizations},
        {"config_screen_key",         GeneralKey::Key,    &GeneralOptions::configScreenKey               },
    };
    return keys;
}

std::string generalValue(GeneralKey const& k) {
    switch (k.type) {
    case GeneralKey::Bool: return *static_cast<bool*>(k.ptr) ? "true" : "false";
    case GeneralKey::Double: {
        std::ostringstream s;
        s << *static_cast<double*>(k.ptr);
        return s.str();
    }
    case GeneralKey::Color: return hex(*static_cast<std::uint32_t*>(k.ptr), 8);
    case GeneralKey::Key: return std::string(1, *static_cast<char*>(k.ptr));
    }
    return "";
}

std::vector<BaseModule::Ptr>* sideList(std::string const& side) {
    if (side == "left" || side == "l") return &BaseModule::modules;
    if (side == "right" || side == "r") return &BaseModule::modulesRight;
    return nullptr;
}

void listModules() {
    auto dump = [](char const* label, std::vector<BaseModule::Ptr> const& list) {
        say(std::string(label) + ":");
        for (size_t i = 0; i < list.size(); ++i) {
            auto const& m = *list[i];
            say("  " + std::to_string(i) + ": " + m.id + " (" + m.toString() + ")" + (m.enabled ? "" : " [off]") + " name=" + hex(m.nameColor, 6) + " value=" + hex(m.valueColor, 6));
        }
    };
    dump("left", BaseModule::modules);
    dump("right", BaseModule::modulesRight);
}

bool patchModule(BaseModule& m, std::string const& key, std::string const& value) {
    Json cfg = Json::object();
    m.saveConfig(cfg);
    auto it = cfg.find(key);
    if (it == cfg.end()) return false;
    if (it->is_boolean()) {
        auto b = parseBool(value);
        if (!b) return false;
        *it = *b;
    } else if (it->is_number_integer()) {
        auto c = parseColor(value);
        if (!c) return false;
        *it = *c;
    } else if (it->is_number()) {
        auto d = parseDouble(value);
        if (!d) return false;
        *it = *d;
    } else if (it->is_string()) {
        *it = value;
    } else {
        return false;
    }
    m.loadConfig(cfg);
    m.markDirty();
    return true;
}

void moduleCommand(std::vector<std::string> const& a) {
    if (a.size() < 2 || a[1] == "list") return listModules();
    auto* list = sideList(a[1]);
    if (!list || a.size() < 3) return fail(t("usage.module"));
    auto& l = *list;
    if (a[2] == "add") {
        if (a.size() < 4) return fail(t("usage.add"));
        auto m = BaseModule::create(a[3]);
        if (!m) return fail(t("unknown_module", {a[3]}));
        if (!BaseModule::moduleById(a[3])) BaseModule::allModules.push_back(m);
        size_t pos = l.size();
        if (a.size() >= 5) pos = std::min<size_t>(std::strtoul(a[4].c_str(), nullptr, 10), l.size());
        l.insert(l.begin() + static_cast<std::ptrdiff_t>(pos), m);
    } else if (a[2] == "remove") {
        if (a.size() < 4) return fail(t("usage.remove"));
        size_t i = std::strtoul(a[3].c_str(), nullptr, 10);
        if (i >= l.size()) return fail(t("out_of_range"));
        l.erase(l.begin() + static_cast<std::ptrdiff_t>(i));
    } else if (a[2] == "move") {
        if (a.size() < 5) return fail(t("usage.move"));
        size_t from = std::strtoul(a[3].c_str(), nullptr, 10), to = std::strtoul(a[4].c_str(), nullptr, 10);
        if (from >= l.size() || to >= l.size()) return fail(t("out_of_range"));
        auto m = l[from];
        l.erase(l.begin() + static_cast<std::ptrdiff_t>(from));
        l.insert(l.begin() + static_cast<std::ptrdiff_t>(to), m);
    } else {
        size_t i = std::strtoul(a[2].c_str(), nullptr, 10);
        if (i >= l.size() || a.size() < 4) return fail(t("usage.op"));
        auto& m = *l[i];
        if (a[3] == "enable" || a[3] == "disable") {
            m.enable(a[3] == "enable");
        } else if (a[3] == "line") {
            if (a.size() < 6) return fail(t("usage.line"));
            auto* line = m.line(a[4]);
            auto  b    = parseBool(a[5]);
            if (!line || !b) return fail(t("bad_line"));
            line->enabled = *b;
            m.markDirty();
        } else if (a[3] == "set") {
            if (a.size() < 6) return fail(t("usage.module_set"));
            if (!patchModule(m, a[4], a[5])) return fail(t("bad_key_value", {a[4]}));
        } else {
            return fail(t("unknown_op", {a[3]}));
        }
    }
    ModConfigFile::save();
    ok(t("module_saved"));
}

void execute(std::vector<std::string> a) {
    if (a.empty() || a[0] == "help") {
        ok(t("help"));
        return;
    }
    auto const& sub = a[0];
    if (sub == "toggle") {
        DebugOverlay::toggleOverlay();
        ok(std::string("overlay = ") + (DebugOverlay::isOverlayVisible ? "true" : "false"));
    } else if (sub == "reload") {
        Modules::resetLayout();
        ModConfigFile::load();
        ok(t("reloaded"));
    } else if (sub == "save") {
        ModConfigFile::save();
        ok(t("saved"));
    } else if (sub == "reset") {
        GeneralOptions::resetToDefaults();
        Modules::resetLayout();
        ModConfigFile::save();
        ok(t("reset"));
    } else if (sub == "get") {
        for (auto const& k : generalKeys()) {
            if (a.size() < 2 || a[1] == k.key) say(std::string(k.key) + " = " + generalValue(k));
        }
    } else if (sub == "set") {
        if (a.size() < 3) return fail(t("usage.set"));
        for (auto const& k : generalKeys()) {
            if (a[1] != k.key) continue;
            switch (k.type) {
            case GeneralKey::Bool: {
                auto b = parseBool(a[2]);
                if (!b) return fail(t("need_bool"));
                *static_cast<bool*>(k.ptr) = *b;
                break;
            }
            case GeneralKey::Double: {
                auto d = parseDouble(a[2]);
                if (!d) return fail(t("need_number"));
                *static_cast<double*>(k.ptr) = std::clamp(*d, k.min, k.max);
                break;
            }
            case GeneralKey::Color: {
                auto c = parseColor(a[2]);
                if (!c) return fail(t("need_color"));
                *static_cast<std::uint32_t*>(k.ptr) = static_cast<std::uint32_t>(*c);
                break;
            }
            case GeneralKey::Key: {
                if (a[2].size() != 1 || !std::isalnum(static_cast<unsigned char>(a[2][0]))) return fail(t("need_key"));
                *static_cast<char*>(k.ptr) = static_cast<char>(std::toupper(static_cast<unsigned char>(a[2][0])));
                break;
            }
            }
            ModConfigFile::save();
            return ok(std::string(k.key) + " = " + generalValue(k));
        }
        fail(t("unknown_key", {a[1]}));
    } else if (sub == "module") {
        moduleCommand(a);
    } else if (sub == "lang") {
        Lang::get().force(a.size() >= 2 ? a[1] : "");
        BaseModule::markAllDirty();
        ok(t("lang", {Lang::get().code()}));
    } else {
        fail(t("unknown_arg", {sub}));
    }
}

bool handle(std::string const& raw) {
    std::string cmd  = (!raw.empty() && raw[0] == '/') ? raw.substr(1) : raw;
    auto        args = split(cmd);
    if (args.empty() || args[0] != "betterf3") return false;
    args.erase(args.begin());
    ll::mod::NativeMod::current()->getLogger().info("BetterF3: [command] {}", cmd);
    gOut.clear();
    gToChat = true;
    execute(args);
    return true;
}

bool                                gRegistered = false;
std::vector<ll::event::ListenerPtr> gListeners;

bool tryRegister() {
    if (gRegistered) return true;
    if (!ll::service::getCommandRegistry(true)) return false;
    auto& cmd = ll::command::CommandRegistrar::getInstance(true).getOrCreateCommand("betterf3", "BetterF3 (unofficial Bedrock port)", CommandPermissionLevel::Any);
    cmd.overload().execute([](CommandOrigin const&, CommandOutput&) {});
    cmd.overload<BetterF3CommandArgs>().required("args").execute([](CommandOrigin const&, CommandOutput&, BetterF3CommandArgs const&) {});
    gRegistered = true;
    return true;
}

}

LL_TYPE_INSTANCE_HOOK(BetterF3SendToServerHook, ll::memory::HookPriority::Normal, LoopbackPacketSender, &LoopbackPacketSender::$sendToServer, void, ::Packet& packet) {
    if (packet.getId() == MinecraftPacketIds::CommandRequest) {
        auto const& req = static_cast<::CommandRequestPacket const&>(packet);
        if (handle(req.mCommand.get())) return;
    }
    NetStats::countSent();
    origin(packet);
}

std::vector<std::string> run(std::string const& args, bool toChat) {
    std::string s = args;
    if (!s.empty() && s[0] == '/') s = s.substr(1);
    auto a = split(s);
    if (!a.empty() && a[0] == "betterf3") a.erase(a.begin());
    gOut.clear();
    gToChat = toChat;
    execute(a);
    gToChat = true;
    return gOut;
}

bool install() {
    if (!tryRegister()) {
        auto& bus = ll::event::EventBus::getInstance();
        gListeners.push_back(bus.emplaceListener<ll::event::ClientStartJoinLevelEvent>([](ll::event::ClientStartJoinLevelEvent&) { tryRegister(); }));
        gListeners.push_back(bus.emplaceListener<ll::event::ClientJoinLevelEvent>([](ll::event::ClientJoinLevelEvent&) { tryRegister(); }));
    }
    return BetterF3SendToServerHook::hook() == 0;
}

void uninstall() {
    auto& bus = ll::event::EventBus::getInstance();
    for (auto& l : gListeners) bus.removeListener(l);
    gListeners.clear();
    BetterF3SendToServerHook::unhook();
}

}
