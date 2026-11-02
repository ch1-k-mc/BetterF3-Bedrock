// SPDX-License-Identifier: MIT
#pragma once

#include "modules/BaseModule.h"

namespace better_f3 {

class EntityModule : public BaseModule {
public:
    TextColor       totalColor;
    TextColor const defaultTotalColor = legacyColor(ChatFormatting::GOLD);

    EntityModule();
    void update(ClientInstance& client) override;

protected:
    void loadModuleConfig(Json const& c) override;
    void saveModuleConfig(Json& c) override;
};

class SoundModule : public BaseModule {
public:
    TextColor       maximumColor;
    TextColor const defaultMaximumColor = legacyColor(ChatFormatting::DARK_AQUA);

    SoundModule();
    void update(ClientInstance& client) override;

protected:
    void loadModuleConfig(Json const& c) override;
    void saveModuleConfig(Json& c) override;
};

class SystemModule : public BaseModule {
public:
    bool const        defaultMemoryColorToggle = true;
    bool              memoryColorToggle;
    std::string const defaultTimeFormat = "HH:mm:ss";
    std::string       timeFormat;

    SystemModule();
    void update(ClientInstance& client) override;

protected:
    void loadModuleConfig(Json const& c) override;
    void saveModuleConfig(Json& c) override;

private:
    long long allocationRate(long long usedBytes);
    long long mLastCalculated = 0;
    long long mAllocatedBytes = -1;
    long long mAllocationRate = 0;
};

class TargetModule : public BaseModule {
public:
    TargetModule();
    void update(ClientInstance& client) override;
};

class MiscLeftModule : public BaseModule {
public:
    MiscLeftModule();
    void update(ClientInstance& client) override;
};

class MiscRightModule : public BaseModule {
public:
    MiscRightModule();
    void update(ClientInstance& client) override;
};

}
