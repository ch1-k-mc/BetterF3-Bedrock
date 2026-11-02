// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/modules/BaseModule.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#include "modules/BaseModule.h"

#include <algorithm>
#include <set>
#include <unordered_set>

#include "config/GeneralOptions.h"
#include "utils/Lang.h"

namespace better_f3 {

std::vector<BaseModule::Ptr> BaseModule::modules;
std::vector<BaseModule::Ptr> BaseModule::modulesRight;
std::vector<BaseModule::Ptr> BaseModule::allModules;

BaseModule::BaseModule(std::string moduleId) : id(std::move(moduleId)) {}

std::map<std::string, BaseModule::Factory>& BaseModule::factories() {
    static std::map<std::string, Factory> f;
    return f;
}

void BaseModule::registerType(std::string const& moduleId, Factory factory) { factories()[moduleId] = std::move(factory); }

BaseModule::Ptr BaseModule::create(std::string const& moduleId) {
    auto it = factories().find(moduleId);
    if (it == factories().end()) return nullptr;
    return it->second();
}

void BaseModule::init(Ptr const& module, PositionEnum position) {
    switch (position) {
    case PositionEnum::RIGHT: modulesRight.push_back(module); break;
    case PositionEnum::LEFT: modules.push_back(module); break;
    case PositionEnum::BOTH:
        modulesRight.push_back(module);
        modules.push_back(module);
        break;
    }
    allModules.push_back(module);
}

void BaseModule::init(Ptr const& module) {
    modules.push_back(module);
    allModules.push_back(module);
}

std::vector<Component> BaseModule::linesFormatted(bool reducedDebug) {
    std::vector<Component> out;
    for (auto& line : mLines) {
        if (reducedDebug && !line->inReducedDebug) continue;
        if (!line->active || !line->enabled) continue;
        if (auto* list = dynamic_cast<DebugLineList*>(line.get())) {
            auto texts = list->toTexts(nameColor, valueColor);
            out.insert(out.end(), texts.begin(), texts.end());
            continue;
        }
        if (!line->isCustom) out.push_back(line->toText(nameColor, valueColor));
        else out.push_back(line->toTextCustom(nameColor));
    }
    return out;
}

DebugLine* BaseModule::line(std::string const& lineId) {
    for (auto& l : mLines) {
        if (l->id() == lineId) return l.get();
    }
    return nullptr;
}

BaseModule::Ptr BaseModule::moduleById(std::string const& moduleId) {
    for (auto& m : allModules) {
        if (m->id == moduleId) return m;
    }
    return nullptr;
}

std::string BaseModule::toString() const { return Lang::get().get("text.betterf3.module." + id); }

void BaseModule::enable(bool e) {
    enabled = e;
    markDirty();
}

void BaseModule::markAllDirty() {
    for (auto const* list : {&allModules, &modules, &modulesRight}) {
        for (auto& m : *list) m->markDirty();
    }
}

bool BaseModule::needsUpdate() const { return !GeneralOptions::enablePerformanceOptimizations || mDirty; }

void BaseModule::refreshDirtyFromState() {
    if (!GeneralOptions::enablePerformanceOptimizations) return;
    std::size_t const next = stateHash();
    if (!mHasCachedStateHash || mCachedStateHash != next) {
        mCachedStateHash    = next;
        mHasCachedStateHash = true;
        mDirty              = true;
    }
}

std::size_t BaseModule::stateHash() const {
    std::size_t result = enabled ? 1231 : 1237;
    result             = 31 * result + nameColor;
    result             = 31 * result + valueColor;
    for (auto const& l : mLines) result = 31 * result + l->cacheStateHash();
    return result;
}

std::vector<Component> const& BaseModule::cachedLinesFormatted(bool reducedDebug) {
    if (!GeneralOptions::enablePerformanceOptimizations) {
        mCachedLines = linesFormatted(reducedDebug);
        return mCachedLines;
    }
    if (mDirty || mCachedReducedDebug != reducedDebug) {
        mCachedLines        = linesFormatted(reducedDebug);
        mCachedReducedDebug = reducedDebug;
        mDirty              = false;
    }
    return mCachedLines;
}

TextColor BaseModule::readColor(Json const& config, char const* key, TextColor defaultColor) {
    if (auto it = config.find(key); it != config.end() && it->is_number_integer()) {
        return static_cast<TextColor>(it->get<std::int64_t>()) & 0xFFFFFF;
    }
    return defaultColor;
}

void BaseModule::writeColor(Json& config, char const* key, TextColor color) { config[key] = static_cast<std::int64_t>(color); }

void BaseModule::loadConfig(Json const& moduleConfig) {
    if (auto it = moduleConfig.find("lines"); it != moduleConfig.end() && it->is_object()) {
        for (auto& [key, value] : it->items()) {
            if (auto* l = line(key); l && value.is_boolean()) l->enabled = value.get<bool>();
        }
    }
    nameColor  = readColor(moduleConfig, "name_color", defaultNameColor);
    valueColor = readColor(moduleConfig, "value_color", defaultValueColor);
    loadModuleConfig(moduleConfig);
    enabled = moduleConfig.value("enabled", true);
}

void BaseModule::saveConfig(Json& moduleConfig) {
    Json lineConfig = Json::object();
    for (auto const& l : mLines) lineConfig[l->id()] = l->enabled;
    moduleConfig["name"] = id;
    writeColor(moduleConfig, "name_color", nameColor);
    writeColor(moduleConfig, "value_color", valueColor);
    saveModuleConfig(moduleConfig);
    moduleConfig["enabled"] = enabled;
    moduleConfig["lines"]   = lineConfig;
}

std::vector<BaseModule::Ptr> BaseModule::distinctModules() {
    std::vector<Ptr>      out;
    std::set<std::string> seen;
    for (auto& m : allModules) {
        if (seen.insert(m->toString()).second) out.push_back(m);
    }
    std::sort(out.begin(), out.end(), [](Ptr const& a, Ptr const& b) { return a->toString() < b->toString(); });
    return out;
}

}
