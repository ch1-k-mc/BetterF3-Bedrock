// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/config/ModConfigFile.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#include "config/ModConfigFile.h"

#include <cctype>
#include <fstream>
#include <sstream>

#include "ll/api/mod/NativeMod.h"

#include "config/GeneralOptions.h"
#include "modules/BaseModule.h"
#include "modules/EmptyModule.h"

namespace better_f3::ModConfigFile {

namespace {

std::filesystem::path gPath;

auto& logger() { return ll::mod::NativeMod::current()->getLogger(); }

BaseModule::Ptr loadModule(Json const& moduleConfig) {
    std::string const name     = moduleConfig.value("name", std::string());
    BaseModule::Ptr   module   = BaseModule::moduleById(name) ? BaseModule::create(name) : nullptr;
    if (!module) module = EmptyModule::make(false);
    module->loadConfig(moduleConfig);
    return module;
}

Json saveModule(BaseModule& module) {
    Json c = Json::object();
    module.saveConfig(c);
    return c;
}

template <class T>
T getOr(Json const& j, char const* key, T def) {
    auto it = j.find(key);
    if (it == j.end()) return def;
    try {
        return it->template get<T>();
    } catch (...) {
        return def;
    }
}

}

void setPath(std::filesystem::path p) { gPath = std::move(p); }
std::filesystem::path const& path() { return gPath; }

void save() {
    Json general = Json::object();
    general["disable_mod"]               = GeneralOptions::disableMod;
    general["auto_start"]                = GeneralOptions::autoF3;
    general["space_modules"]             = GeneralOptions::spaceEveryModule;
    general["shadow_text"]               = GeneralOptions::shadowText;
    general["animations"]                = GeneralOptions::enableAnimations;
    general["animationSpeed"]            = GeneralOptions::animationSpeed;
    general["fontScale"]                 = GeneralOptions::fontScale;
    general["background_color"]          = static_cast<std::int32_t>(GeneralOptions::backgroundColor);
    general["hide_debug_crosshair"]      = GeneralOptions::hideDebugCrosshair;
    general["hide_sidebar"]              = GeneralOptions::hideSidebar;
    general["hide_bossbar"]              = GeneralOptions::hideBossbar;
    general["always_show_profiler"]      = GeneralOptions::alwaysEnableProfiler;
    general["always_show_tps"]           = GeneralOptions::alwaysEnableTPS;
    general["always_show_ping"]          = GeneralOptions::alwaysEnablePing;
    general["performance_optimizations"] = GeneralOptions::enablePerformanceOptimizations;
    general["config_screen_key"]         = std::string(1, GeneralOptions::configScreenKey);

    Json left  = Json::array();
    Json right = Json::array();
    for (auto& m : BaseModule::modules) left.push_back(saveModule(*m));
    for (auto& m : BaseModule::modulesRight) right.push_back(saveModule(*m));

    Json config            = Json::object();
    config["modules_left"]  = left;
    config["modules_right"] = right;
    config["general"]       = general;
    try {
        std::filesystem::create_directories(gPath.parent_path());
        std::ofstream out(gPath, std::ios::binary | std::ios::trunc);
        out << config.dump(2);
    } catch (std::exception const& e) {
        logger().warn("BetterF3: failed to save the config: {}", e.what());
    }
    BaseModule::markAllDirty();
}

void load() {
    if (!std::filesystem::exists(gPath)) return;
    std::ifstream     in(gPath, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    Json config = Json::parse(ss.str(), nullptr, false);
    if (!config.is_object()) {
        logger().warn("BetterF3: cannot read the config file (not JSON): {}", gPath.filename().string());
        return;
    }

    auto const allModulesConfig = config.find("modules");
    bool const oldFormat        = allModulesConfig != config.end() && allModulesConfig->is_object();
    if (oldFormat) {
        for (auto& module : BaseModule::allModules) {
            if (auto it = allModulesConfig->find(module->id); it != allModulesConfig->end()) module->loadConfig(*it);
        }
    } else {
        std::vector<BaseModule::Ptr> left;
        std::vector<BaseModule::Ptr> right;
        auto readList = [&](char const* key, std::vector<BaseModule::Ptr>& out) {
            auto it = config.find(key);
            if (it == config.end() || !it->is_array()) return;
            for (auto& mc : *it) {
                if (mc.is_object() && mc.contains("name")) out.push_back(loadModule(mc));
            }
        };
        readList("modules_left", left);
        readList("modules_right", right);
        if (!left.empty() || !right.empty()) {
            BaseModule::modules      = left;
            BaseModule::modulesRight = right;
        }
    }

    if (auto gi = config.find("general"); gi != config.end() && gi->is_object()) {
        Json const& general = *gi;
        if (oldFormat) {
            auto order = [&](char const* key, std::vector<BaseModule::Ptr>& target) {
                std::vector<BaseModule::Ptr> list;
                if (auto it = general.find(key); it != general.end() && it->is_array()) {
                    for (auto& s : *it) {
                        if (!s.is_string()) continue;
                        if (auto m = BaseModule::moduleById(s.get<std::string>())) list.push_back(m);
                    }
                }
                if (!list.empty()) target = list;
            };
            order("modules_left_order", BaseModule::modules);
            order("modules_right_order", BaseModule::modulesRight);
        }
        GeneralOptions::disableMod                     = getOr(general, "disable_mod", false);
        GeneralOptions::autoF3                         = getOr(general, "auto_start", false);
        GeneralOptions::spaceEveryModule               = getOr(general, "space_modules", false);
        GeneralOptions::shadowText                     = getOr(general, "shadow_text", true);
        GeneralOptions::enableAnimations               = getOr(general, "animations", true);
        GeneralOptions::animationSpeed                 = getOr(general, "animationSpeed", 1.0);
        GeneralOptions::fontScale                      = getOr(general, "fontScale", 1.0);
        GeneralOptions::backgroundColor                = static_cast<std::uint32_t>(getOr<std::int64_t>(general, "background_color", 0x6F505050));
        GeneralOptions::hideDebugCrosshair             = getOr(general, "hide_debug_crosshair", false);
        GeneralOptions::hideSidebar                    = getOr(general, "hide_sidebar", true);
        GeneralOptions::hideBossbar                    = getOr(general, "hide_bossbar", true);
        GeneralOptions::alwaysEnableProfiler           = getOr(general, "always_show_profiler", false);
        GeneralOptions::alwaysEnableTPS                = getOr(general, "always_show_tps", false);
        GeneralOptions::alwaysEnablePing               = getOr(general, "always_show_ping", false);
        GeneralOptions::enablePerformanceOptimizations = getOr(general, "performance_optimizations", true);
        {
            std::string const k = getOr<std::string>(general, "config_screen_key", "M");
            char const        c = k.size() == 1 ? static_cast<char>(std::toupper(static_cast<unsigned char>(k[0]))) : 'M';
            GeneralOptions::configScreenKey = std::isalnum(static_cast<unsigned char>(c)) ? c : 'M';
        }
    }
    BaseModule::markAllDirty();
    logger().info("BetterF3: config loaded ({} left / {} right modules)", BaseModule::modules.size(), BaseModule::modulesRight.size());
}

}
