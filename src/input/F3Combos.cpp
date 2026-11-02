// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/mixin/KeyboardMixin.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#include "input/F3Combos.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <vector>

#include "ll/api/Versions.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/input/KeyInputEvent.h"
#include "ll/api/event/render/UIRenderEvent.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/game/MinecraftGame.h"
#include "mc/client/gui/GuiData.h"
#include "mc/client/gui/screens/interfaces/ISceneStack.h"
#include "mc/client/options/IOptionRegistry.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/client/renderer/game/LevelRenderer.h"
#include "mc/common/SharedConstants.h"
#include "mc/common/SubClientId.h"
#include "mc/deps/application/AppPlatform.h"
#include "mc/deps/nbt/ByteTag.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/deps/nbt/StringTag.h"
#include "mc/network/PacketSender.h"
#include "mc/network/packet/CommandRequestPacket.h"
#include "mc/network/packet/CommandRequestPacketPayload.h"
#include "mc/options/option_types/IntOption.h"
#include "mc/options/option_types/Option.h"
#include "mc/options/option_types/OptionID.h"
#include "mc/options/option_types/OptionType.h"
#include "mc/profile/ProfilerLite.h"
#include "mc/server/commands/CommandContext.h"
#include "mc/server/commands/CommandOriginData.h"
#include "mc/server/commands/PlayerCommandOrigin.h"
#include "mc/server/commands/CommandOriginType.h"
#include "mc/server/commands/CurrentCmdVersion.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/item/Item.h"
#include "mc/world/item/ItemStackBase.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/GameType.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/dimension/VanillaDimensions.h"
#include "mc/world/phys/HitResult.h"
#include "mc/safety/RedactableString.h"

#include "config/GeneralOptions.h"
#include "config/ModConfigFile.h"
#include "gui/ConfigScreens.h"
#include "gui/Gui.h"
#include "hooks/DebugHooks.h"
#include "modules/Modules.h"
#include "render/Gui2d.h"
#include "utils/DebugOverlay.h"
#include "utils/Lang.h"
#include "utils/ServerSampler.h"
#include "utils/SystemInfo.h"
#include "utils/Utils.h"

namespace better_f3::F3Combos {

namespace {

constexpr int VK_1 = 49, VK_2 = 50, VK_3 = 51, VK_4 = 52;
constexpr int VK_ESC = 27, VK_F4 = 115, VK_F6 = 117;

bool        gAdvancedTooltips = false;
bool        gChunkBorders     = false;
bool        gHitboxes         = false;
bool        gPauseOnLostFocus = true;
std::string gClipboard;
GameType    gPreviousGameType = GameType::Undefined;

struct Profile {
    bool                running = false;
    std::int64_t        started = 0;
    std::vector<float>  frameMs;
    std::vector<float>  tickMs;
} gProfile;
constexpr int kProfileSeconds = 10;

bool gSwitcherOpen = false;
int  gSwitcherSel  = 0;
constexpr std::array<GameType, 4> kModes{GameType::Creative, GameType::Survival, GameType::Adventure, GameType::Spectator};
constexpr std::array<char const*, 4> kModeKeys{"betterf3.gamemode.creative", "betterf3.gamemode.survival", "betterf3.gamemode.adventure", "betterf3.gamemode.spectator"};
constexpr std::array<char const*, 4> kModeCommand{"creative", "survival", "adventure", "spectator"};

auto& logger() { return ll::mod::NativeMod::current()->getLogger(); }
std::string tr(std::string const& key) { return Lang::get().get(key); }

ClientInstance* client() {
    auto ci = ll::service::getClientInstance();
    return ci ? ci.as_ptr() : nullptr;
}

void chat(std::string const& s) {
    if (auto* ci = client()) ci->getGuiData()->displayClientMessage(s, std::nullopt, false);
}

void feedback(std::string const& text) {
    std::string const sec(kSection);
    chat(sec + "e" + sec + "l" + tr("betterf3.debug.prefix") + sec + "r " + text);
}

void copyToClipboard(std::string const& s) {
    gClipboard = s;
    if (auto app = ll::service::getAppPlatform()) app->setClipboard(s);
}

void sendCommand(std::string const& cmd) {
    auto* ci = client();
    if (!ci) return;
    auto* player = ci->getLocalPlayer();
    if (!player) return;
    CommandContext ctx(cmd, std::make_unique<PlayerCommandOrigin>(player->getLevel(), player->getOrCreateUniqueID()), static_cast<int>(CurrentCmdVersion::Latest));
    CommandRequestPacket pkt(CommandRequestPacketPayload(ctx, false));
    ci->getPacketSender().sendToServer(pkt);
}

std::string dimensionName(DimensionType id) {
    std::string d = VanillaDimensions::toString(id);
    std::transform(d.begin(), d.end(), d.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return d;
}

std::string bedrockStates(Block const& b) {
    auto const& root = *b.mSerializationId;
    if (!root.contains("states")) return "";
    auto const& states = root.at("states");
    if (!states.is_object() || states.items().empty()) return "";
    std::string out = " [";
    bool        first = true;
    for (auto const& [name, v] : states.items()) {
        if (!first) out += ",";
        first = false;
        out += "\"" + name + "\"=";
        if (v.is_boolean()) out += v.get<ByteTag>().data != 0 ? "true" : "false";
        else if (v.is_string()) out += "\"" + static_cast<std::string const&>(v.get<StringTag>()) + "\"";
        else out += v.toSnbt(SnbtFormat::Minimize);
    }
    return out + "]";
}

IntOption* viewDistanceOption(IOptionRegistry& opts) {
    static IntOption* cached = nullptr;
    if (cached) return cached;
    for (int id = 0; id < 2048; ++id) {
        auto o = opts.getIfValid(static_cast<OptionID>(id));
        if (!o || !*o) continue;
        Option* opt = *o;
        auto*   impl = opt->mImpl.get();
        if (!impl) continue;
        if (impl->mSaveTag.get() == "gfx_viewdistance" && impl->mOptionType == OptionType::Int) {
            cached = static_cast<IntOption*>(opt);
            break;
        }
    }
    return cached;
}

void cycleRenderDistance(bool shift) {
    auto* ci = client();
    if (!ci) return;
    auto& opts   = ci->getOptions();
    auto* option = viewDistanceOption(opts);
    if (!option) {
        feedback(tr("betterf3.debug.not_supported"));
        return;
    }
    int const chunks = opts.getViewDistanceChunks();
    int const value  = option->mValue;
    int const step   = (value == chunks * 16) ? 16 : 1;
    int const minV   = std::max<int>(option->VALUE_MIN, 2 * step);
    int const maxV   = std::min<int>(option->VALUE_MAX, std::max(opts.getMaxViewDistanceChunks(), 2) * step);
    int const next   = std::clamp(value + (shift ? -step : step), minV, maxV);
    option->set(next, true);
    feedback(utils::i18n("debug.betterf3.cycle_renderdistance.message", {std::to_string(next / step)}));
}

void copyLocation() {
    auto* ci = client();
    auto* p  = ci ? ci->getLocalPlayer() : nullptr;
    if (!p) return;
    Vec3 const pos = p->getFeetPos();
    Vec2 const rot = p->getRotation();
    char       buf[256];
    std::snprintf(buf, sizeof buf, "/execute in %s run tp @s %.2f %.2f %.2f %.2f %.2f", dimensionName(p->getDimensionId()).c_str(), pos.x, pos.y, pos.z, rot.y, rot.x);
    copyToClipboard(buf);
    feedback(tr("betterf3.debug.copy_location"));
}

void copyTargetData() {
    auto* ci = client();
    auto* p  = ci ? ci->getLocalPlayer() : nullptr;
    if (!p) return;
    HitResult const& hr = p->getLevel().getHitResult();
    if (hr.mType == HitResultType::Entity) {
        if (Actor* e = hr.getEntity()) {
            Vec3 const pos = e->getFeetPos();
            char       buf[256];
            std::snprintf(buf, sizeof buf, "/summon %s %.2f %.2f %.2f", e->getTypeName().c_str(), pos.x, pos.y, pos.z);
            copyToClipboard(buf);
            feedback(tr("betterf3.debug.inspect.entity"));
        }
    } else if (hr.mType == HitResultType::Tile) {
        Block const& b = p->getDimensionBlockSource().getBlock(hr.mBlock);
        copyToClipboard("/setblock " + std::to_string(hr.mBlock.x) + " " + std::to_string(hr.mBlock.y) + " " + std::to_string(hr.mBlock.z) + " " + b.getTypeName() + bedrockStates(b));
        feedback(tr("betterf3.debug.inspect.block"));
    }
}

void toggleSpectator() {
    auto* ci = client();
    auto* p  = ci ? ci->getLocalPlayer() : nullptr;
    if (!p) return;
    GameType const cur = p->getPlayerGameType();
    if (cur != GameType::Spectator) {
        gPreviousGameType = cur;
        sendCommand("/gamemode spectator");
    } else {
        GameType const back = gPreviousGameType == GameType::Undefined || gPreviousGameType == GameType::Spectator ? GameType::Creative : gPreviousGameType;
        for (size_t i = 0; i < kModes.size(); ++i) {
            if (kModes[i] == back) sendCommand(std::string("/gamemode ") + kModeCommand[i]);
        }
    }
}

void showHelp() {
    feedback(tr("betterf3.debug.help.message"));
    for (char const* k : {"a", "b", "c", "d", "g", "h", "i", "l", "n", "p", "q", "v", "charts", "f4", "esc"}) chat(tr(std::string("betterf3.debug.help.") + k));
    chat(tr("debug.betterf3.cycle_renderdistance.help"));
    chat(tr("debug.betterf3.modmenu.help"));
}

void showVersion() {
    feedback(tr("betterf3.debug.version.header"));
    auto const g = ll::getGameVersion();
    chat("Minecraft: " + std::to_string(g.major) + "." + std::to_string(g.minor) + "." + std::to_string(g.patch) + " (protocol " + std::to_string(SharedConstants::NetworkProtocolVersion()) + ")");
    chat("LeviLamina: " + ll::getLoaderVersion().to_string());
    chat("BetterF3: " + utils::modVersion());
    chat("OS: " + SystemInfo::osName());
    chat("Graphics API: " + SystemInfo::graphicsApi());
}

void startOrStopProfiling() {
    if (!gProfile.running) {
        gProfile         = Profile{};
        gProfile.running = true;
        gProfile.started = utils::millis();
        feedback(utils::i18n("betterf3.debug.profiling.start", {std::to_string(kProfileSeconds)}));
        return;
    }
    gProfile.running = false;
    std::time_t const t = std::time(nullptr);
    std::tm           tm{};
    localtime_s(&tm, &t);
    char name[64];
    std::strftime(name, sizeof name, "profile-%Y%m%d-%H%M%S.txt", &tm);
    auto dir = ll::mod::NativeMod::current()->getModDir() / "debug";
    std::filesystem::create_directories(dir);
    auto stat = [](std::vector<float> v, std::ofstream& out, char const* label) {
        if (v.empty()) {
            out << label << ": (no samples)\n";
            return;
        }
        std::sort(v.begin(), v.end());
        double sum = 0;
        for (float x : v) sum += x;
        auto pct = [&](double q) { return v[std::min(v.size() - 1, static_cast<size_t>(q * static_cast<double>(v.size())))]; };
        out << label << ": samples=" << v.size() << " avg=" << sum / static_cast<double>(v.size()) << "ms min=" << v.front() << " p50=" << pct(0.5) << " p95=" << pct(0.95) << " p99=" << pct(0.99) << " max=" << v.back() << "\n";
    };
    std::ofstream out(dir / name);
    double const seconds = static_cast<double>(utils::millis() - gProfile.started) / 1000.0;
    out << "BetterF3 profile (" << seconds << " s)\n";
    out << "frames=" << gProfile.frameMs.size() << " avgFps=" << (seconds > 0 ? static_cast<double>(gProfile.frameMs.size()) / seconds : 0) << "\n";
    stat(gProfile.frameMs, out, "frame");
    stat(gProfile.tickMs, out, "serverTick");
    feedback(utils::i18n("betterf3.debug.profiling.stop", {std::string("mods/BetterF3/debug/") + name}));
}

void openOrCycleSwitcher() {
    auto* ci = client();
    auto* p  = ci ? ci->getLocalPlayer() : nullptr;
    if (!p) return;
    if (!gSwitcherOpen) {
        GameType const cur  = p->getPlayerGameType();
        GameType const pick = gPreviousGameType != GameType::Undefined && gPreviousGameType != cur ? gPreviousGameType : GameType::Undefined;
        gSwitcherSel        = 0;
        for (size_t i = 0; i < kModes.size(); ++i) {
            if (pick != GameType::Undefined ? kModes[i] == pick : kModes[i] == cur) gSwitcherSel = static_cast<int>(i);
        }
        if (pick == GameType::Undefined) gSwitcherSel = (gSwitcherSel + 1) % 4;
        gSwitcherOpen = true;
    } else {
        gSwitcherSel = (gSwitcherSel + 1) % 4;
    }
}

void applySwitcher() {
    if (!gSwitcherOpen) return;
    gSwitcherOpen = false;
    auto* ci      = client();
    auto* p       = ci ? ci->getLocalPlayer() : nullptr;
    if (!p) return;
    GameType const cur = p->getPlayerGameType();
    if (kModes[gSwitcherSel] == cur) return;
    gPreviousGameType = cur;
    sendCommand(std::string("/gamemode ") + kModeCommand[gSwitcherSel]);
}

void pauseWithoutMenu() {
    auto* ci = client();
    if (!ci) return;
    auto& mg = static_cast<MinecraftGame&>(ci->getMinecraftGame());
    mg.requestInGamePause(SubClientId::PrimaryClient, !mg._isGameplayPaused());
}

}

namespace {
bool         gFocused       = true;
std::int64_t gGainedAt      = -1000000;
std::int64_t gSuppressedAt  = -1000000;
std::int64_t gLastDismissAt = -1000000;
std::int64_t gEscAt         = -1000000;

bool suppressFocusPause() { return !gPauseOnLostFocus || gui::GuiManager::get().isOpen(); }
bool focusRelated() { return (!gFocused || utils::millis() - gGainedAt < 3000) && utils::millis() - gEscAt > 500; }
bool shouldSuppress(bool status) {
    if (!status || !suppressFocusPause() || !focusRelated()) return false;
    gSuppressedAt = utils::millis();
    return true;
}
}

LL_TYPE_INSTANCE_HOOK(BetterF3FocusLostHook, ll::memory::HookPriority::Normal, MinecraftGame, &MinecraftGame::$onAppFocusLost, void) {
    gFocused = false;
    origin();
}

LL_TYPE_INSTANCE_HOOK(BetterF3FocusGainedHook, ll::memory::HookPriority::Normal, MinecraftGame, &MinecraftGame::$onAppFocusGained, void) {
    gFocused  = true;
    gGainedAt = utils::millis();
    origin();
}

LL_TYPE_INSTANCE_HOOK(
    BetterF3RequestPauseHook,
    ll::memory::HookPriority::Normal,
    MinecraftGame,
    &MinecraftGame::$requestInGamePause,
    bool,
    ::SubClientId const& subClient,
    bool                 status
) {
    if (shouldSuppress(status)) return false;
    return origin(subClient, status);
}

LL_TYPE_INSTANCE_HOOK(
    BetterF3ClientRequestPauseHook,
    ll::memory::HookPriority::Normal,
    ClientInstance,
    &ClientInstance::$requestInGamePause,
    bool,
    bool status
) {
    if (shouldSuppress(status)) return false;
    return origin(status);
}

namespace {
void dismissFocusPauseScreen() {
    std::int64_t const now = utils::millis();
    if (now - gSuppressedAt > 2000 || now - gLastDismissAt < 500 || !focusRelated()) return;
    auto* ci = client();
    if (!ci || ci->getTopScreenName() != "pause_screen") return;
    gLastDismissAt = now;
    ci->getClientSceneStack()->schedulePopScreenWithExpectedNames(std::vector<std::string>{"pause_screen"});
}
ll::event::ListenerPtr gPauseListener;
ll::event::ListenerPtr gEscListener;
}

LL_TYPE_INSTANCE_HOOK(
    BetterF3HovertextHook,
    ll::memory::HookPriority::Normal,
    Item,
    &Item::$appendFormattedHovertext,
    void,
    ::ItemStackBase const&               stack,
    ::Level&                             level,
    ::Bedrock::Safety::RedactableString& hovertext,
    bool                                 showCategory
) {
    origin(stack, level, hovertext, showCategory);
    if (!gAdvancedTooltips) return;
    std::string const sec(kSection);
    short const       max = getMaxDamage();
    if (max > 0) {
        int const left = max - stack.getDamageValue();
        hovertext += "\n" + sec + "r" + utils::i18n("betterf3.tooltip.durability", {std::to_string(left), std::to_string(max)});
    }
    hovertext += "\n" + sec + "8" + getSerializedName();
}

bool advancedTooltips() { return gAdvancedTooltips; }
bool chunkBorders() { return gChunkBorders; }
bool hitboxes() { return gHitboxes; }
bool pauseOnLostFocus() { return gPauseOnLostFocus; }
bool switcherOpen() { return gSwitcherOpen; }
bool profiling() { return gProfile.running; }
std::string const& lastClipboard() { return gClipboard; }

bool press(int key, bool shift, bool ctrl) {
    if (key == GeneralOptions::configScreenKey) {
        gui::openConfigScreen();
        return true;
    }
    switch (key) {
    case 'A':
        if (auto* ci = client(); ci && ci->getLevelRenderer()) ci->getLevelRenderer()->rebuildAllRenderChunkGeometry();
        feedback(tr("betterf3.debug.reload_chunks"));
        return true;
    case 'B':
        gHitboxes = !gHitboxes;
        feedback(tr(gHitboxes ? "betterf3.debug.hitboxes.on" : "betterf3.debug.hitboxes.off"));
        return true;
    case 'C': copyLocation(); return true;
    case 'D':
        if (auto* ci = client()) {
            auto gd = ci->getGuiData();
            gd->clearPlayerMessages();
            gd->mGuiMessages->clear();
        }
        return true;
    case 'F':
        if (ctrl) feedback(tr("betterf3.debug.not_supported"));
        else cycleRenderDistance(shift);
        return true;
    case 'G':
        gChunkBorders = !gChunkBorders;
        feedback(tr(gChunkBorders ? "betterf3.debug.chunk_boundaries.on" : "betterf3.debug.chunk_boundaries.off"));
        return true;
    case 'H':
        gAdvancedTooltips = !gAdvancedTooltips;
        feedback(tr(gAdvancedTooltips ? "betterf3.debug.advanced_tooltips.on" : "betterf3.debug.advanced_tooltips.off"));
        return true;
    case 'I': copyTargetData(); return true;
    case 'L': startOrStopProfiling(); return true;
    case 'N': toggleSpectator(); return true;
    case 'P':
        gPauseOnLostFocus = !gPauseOnLostFocus;
        feedback(tr(gPauseOnLostFocus ? "betterf3.debug.pause_focus.on" : "betterf3.debug.pause_focus.off"));
        return true;
    case 'Q': showHelp(); return true;
    case 'V': showVersion(); return true;
    case VK_1: DebugOverlay::renderProfilerChart = !DebugOverlay::renderProfilerChart; return true;
    case VK_2: DebugOverlay::renderFpsCharts = !DebugOverlay::renderFpsCharts; return true;
    case VK_3: DebugOverlay::renderNetworkCharts = !DebugOverlay::renderNetworkCharts; return true;
    case VK_F4: openOrCycleSwitcher(); return true;
    case VK_ESC: pauseWithoutMenu(); return true;
    case 'S':
    case 'T':
    case 'X':
    case VK_4:
    case VK_F6:
        feedback(tr("betterf3.debug.not_supported"));
        return true;
    default: return false;
    }
}

void releaseF3() { applySwitcher(); }

void onFrame(Gui2d& g) {
    if (gProfile.running) {
        gProfile.frameMs.push_back(ProfilerLite::gProfilerLiteInstance().mFrameData->mFrameTime);
        if (auto s = ServerSampler::get(); s.valid) gProfile.tickMs.push_back(s.tickMs);
        ServerSampler::request();
        if (utils::millis() - gProfile.started >= kProfileSeconds * 1000) startOrStopProfiling();
    }
    if (!gSwitcherOpen) return;
    float const cx = g.width() / 2.f;
    float const cy = g.height() / 2.f - 31.f;
    float const w  = 125.f;
    g.fill(cx - w / 2.f, cy - 12.f, cx + w / 2.f, cy + 52.f, 0xB0000000);
    Component const title = Component::literal(tr(kModeKeys[gSwitcherSel]));
    g.text(title, cx - g.width(title) / 2.f, cy - 8.f, 0xFFFFFFFF, true);
    for (int i = 0; i < 4; ++i) {
        float const x = cx - 2.f * 31.f + static_cast<float>(i) * 31.f + 2.f;
        float const y = cy + 8.f;
        g.fill(x, y, x + 26.f, y + 26.f, i == gSwitcherSel ? 0xFFFFFFFF : 0xFF505050);
        g.fill(x + 1.f, y + 1.f, x + 25.f, y + 25.f, 0xFF202020);
        Component const c = Component::literal(std::string(1, static_cast<char>(std::toupper(kModeCommand[i][0]))));
        g.text(c, x + 13.f - g.width(c) / 2.f, y + 9.f, i == gSwitcherSel ? 0xFFFFFF55 : 0xFFE0E0E0, true);
    }
    Component const next = Component::literal(tr("betterf3.gamemode.select_next"));
    g.text(next, cx - g.width(next) / 2.f, cy + 40.f, 0xFF55FFFF, true);
}

bool install() {
    hooks::setComboHandler([](int code) { return press(code, hooks::isShiftDown(), hooks::isCtrlDown()); });
    hooks::setReleaseHandler([](bool) { releaseF3(); });
    bool ok = BetterF3FocusLostHook::hook() == 0;
    ok      = (BetterF3FocusGainedHook::hook() == 0) && ok;
    ok      = (BetterF3RequestPauseHook::hook() == 0) && ok;
    ok      = (BetterF3ClientRequestPauseHook::hook() == 0) && ok;
    gPauseListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::AfterUIRenderEvent>([](ll::event::AfterUIRenderEvent&) { dismissFocusPauseScreen(); });
    gEscListener   = ll::event::EventBus::getInstance().emplaceListener<ll::event::KeyInputEvent>([](ll::event::KeyInputEvent& e) {
        if (e.keyCode() == VK_ESC && e.isDown()) gEscAt = utils::millis();
    });
    ok      = (BetterF3HovertextHook::hook() == 0) && ok;
    return ok;
}

void uninstall() {
    hooks::setComboHandler(nullptr);
    hooks::setReleaseHandler(nullptr);
    if (gPauseListener) ll::event::EventBus::getInstance().removeListener(gPauseListener);
    gPauseListener = nullptr;
    if (gEscListener) ll::event::EventBus::getInstance().removeListener(gEscListener);
    gEscListener = nullptr;
    BetterF3FocusLostHook::unhook();
    BetterF3FocusGainedHook::unhook();
    BetterF3RequestPauseHook::unhook();
    BetterF3ClientRequestPauseHook::unhook();
    BetterF3HovertextHook::unhook();
}

}
