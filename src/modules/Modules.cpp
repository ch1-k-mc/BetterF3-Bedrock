// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 fabric/src/main/java/me/cominixo/betterf3/BetterF3Fabric.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#include "modules/Modules.h"

#include "modules/BaseModule.h"
#include "modules/EmptyModule.h"
#include "modules/HelpModule.h"
#include "modules/LeftModules1.h"
#include "modules/Modules2.h"

namespace better_f3::Modules {

namespace {

template <class T>
void registerType(char const* id) {
    BaseModule::registerType(id, [] { return std::make_shared<T>(); });
}

void layout() {
    BaseModule::modules.clear();
    BaseModule::modulesRight.clear();
    BaseModule::allModules.clear();
    BaseModule::init(std::make_shared<MinecraftModule>());
    BaseModule::init(std::make_shared<FpsModule>());
    BaseModule::init(std::make_shared<GraphicsModule>());
    BaseModule::init(std::make_shared<ServerModule>());
    BaseModule::init(std::make_shared<CoordsModule>());
    BaseModule::init(std::make_shared<ChunksModule>());
    BaseModule::init(std::make_shared<LocationModule>());
    BaseModule::init(std::make_shared<EntityModule>());
    BaseModule::init(std::make_shared<SoundModule>());
    BaseModule::init(std::make_shared<HelpModule>());
    BaseModule::modules.push_back(EmptyModule::make(false));
    BaseModule::init(std::make_shared<MiscLeftModule>());
    BaseModule::init(std::make_shared<SystemModule>(), PositionEnum::RIGHT);
    BaseModule::init(std::make_shared<MiscRightModule>(), PositionEnum::RIGHT);
    BaseModule::modulesRight.push_back(EmptyModule::make(false));
    BaseModule::init(std::make_shared<TargetModule>(), PositionEnum::RIGHT);
}

}

void registerDefaults() {
    registerType<MinecraftModule>("minecraft");
    registerType<FpsModule>("fps");
    registerType<GraphicsModule>("graphics");
    registerType<ServerModule>("server");
    registerType<CoordsModule>("coords");
    registerType<ChunksModule>("chunks");
    registerType<LocationModule>("location");
    registerType<EntityModule>("entity");
    registerType<SoundModule>("sound");
    registerType<HelpModule>("help");
    registerType<MiscLeftModule>("miscleft");
    registerType<SystemModule>("system");
    registerType<MiscRightModule>("miscright");
    registerType<TargetModule>("target");
    BaseModule::registerType("empty", [] { return EmptyModule::make(false); });
    layout();
}

void resetLayout() { layout(); }

}
