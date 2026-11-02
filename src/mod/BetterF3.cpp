// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 fabric/src/main/java/me/cominixo/betterf3/BetterF3Fabric.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#include "mod/BetterF3.h"

#include "ll/api/mod/RegisterHelper.h"

#include "api/Diagnostics.h"
#include "command/BetterF3Command.h"
#include "config/ModConfigFile.h"
#include "gui/Gui.h"
#include "hooks/DebugHooks.h"
#include "hooks/HudHide.h"
#include "hooks/RenderStats.h"
#include "hooks/SoundStats.h"
#include "input/F3Combos.h"
#include "modules/Modules.h"
#include "net/NetStats.h"
#include "render/WorldLines.h"
#include "utils/Lang.h"
#include "utils/Utils.h"

namespace better_f3 {

BetterF3& BetterF3::getInstance() {
    static BetterF3 instance;
    return instance;
}

bool BetterF3::load() {
    getSelf().getLogger().info("[BetterF3] Loading...");
    return true;
}

bool BetterF3::enable() {
    auto& self = getSelf();
    utils::modVersion(self.getManifest().version ? self.getManifest().version->to_string() : "");
    Lang::get().setDir(self.getModDir() / "assets" / "lang");
    Modules::registerDefaults();
    ModConfigFile::setPath(self.getConfigDir() / "betterf3.json");
    ModConfigFile::load();
    hooks::start();
    if (!RenderStats::install()) self.getLogger().warn("BetterF3: failed to hook render statistics");
    if (!SoundStats::install()) self.getLogger().warn("BetterF3: failed to hook sound statistics (fmod.dll)");
    HudHide::start();
    if (!NetStats::install()) self.getLogger().warn("BetterF3: failed to hook some network statistics");
    if (!WorldLines::install()) self.getLogger().warn("BetterF3: failed to hook world line rendering");
    if (!F3Combos::install()) self.getLogger().warn("BetterF3: failed to hook some F3 key actions");
    if (!command::install()) self.getLogger().warn("BetterF3: failed to hook the /betterf3 command");
    if (!gui::GuiManager::get().install()) self.getLogger().warn("BetterF3: failed to hook some settings screen input");
    api::registerDiagnostics();
    self.getLogger().info("[BetterF3] All done! ({})", utils::modVersion());
    return true;
}

bool BetterF3::disable() {
    api::unregisterDiagnostics();
    gui::GuiManager::get().uninstall();
    command::uninstall();
    F3Combos::uninstall();
    HudHide::stop();
    WorldLines::uninstall();
    NetStats::uninstall();
    SoundStats::uninstall();
    RenderStats::uninstall();
    hooks::stop();
    return true;
}

}

LL_REGISTER_MOD(better_f3::BetterF3, better_f3::BetterF3::getInstance());
