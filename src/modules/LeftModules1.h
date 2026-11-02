// SPDX-License-Identifier: MIT
#pragma once

#include "mc/deps/core/math/Vec3.h"

#include "modules/BaseModule.h"

namespace better_f3 {

class MinecraftModule : public BaseModule {
public:
    MinecraftModule();
    void update(ClientInstance& client) override;
    [[nodiscard]] bool updatesEveryFrame() const override { return false; }
};

class FpsModule : public BaseModule {
public:
    TextColor       colorHigh, colorMed, colorLow;
    TextColor const defaultColorHigh = legacyColor(ChatFormatting::GREEN);
    TextColor const defaultColorMed  = legacyColor(ChatFormatting::YELLOW);
    TextColor const defaultColorLow  = legacyColor(ChatFormatting::RED);

    FpsModule();
    void update(ClientInstance& client) override;

protected:
    void loadModuleConfig(Json const& c) override;
    void saveModuleConfig(Json& c) override;
};

class GraphicsModule : public BaseModule {
public:
    GraphicsModule();
    void update(ClientInstance& client) override;
    [[nodiscard]] bool updatesEveryFrame() const override { return false; }
};

class ServerModule : public BaseModule {
public:
    ServerModule();
    void update(ClientInstance& client) override;
};

class CoordsModule : public BaseModule {
public:
    static constexpr char const* FORMAT_COORDS = "format.betterf3.coords";
    TextColor       colorX, colorY, colorZ;
    TextColor const defaultColorX = legacyColor(ChatFormatting::RED);
    TextColor const defaultColorY = legacyColor(ChatFormatting::GREEN);
    TextColor const defaultColorZ = legacyColor(ChatFormatting::AQUA);

    CoordsModule();
    void update(ClientInstance& client) override;

protected:
    void loadModuleConfig(Json const& c) override;
    void saveModuleConfig(Json& c) override;

private:
    Vec3          mPrevPos{0, 0, 0};
    Vec3          mVelocity{0, 0, 0};
    std::uint64_t mPositionUpdateTime = 0;
};

class ChunksModule : public BaseModule {
public:
    TextColor       totalColor, enabledColor, disabledColor;
    TextColor const defaultTotalColor    = legacyColor(ChatFormatting::GOLD);
    TextColor const defaultEnabledColor  = legacyColor(ChatFormatting::GREEN);
    TextColor const defaultDisabledColor = legacyColor(ChatFormatting::RED);

    ChunksModule();
    void update(ClientInstance& client) override;

protected:
    void loadModuleConfig(Json const& c) override;
    void saveModuleConfig(Json& c) override;
};

class LocationModule : public BaseModule {
public:
    LocationModule();
    void update(ClientInstance& client) override;
};

}
