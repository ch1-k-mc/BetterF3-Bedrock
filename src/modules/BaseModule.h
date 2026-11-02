// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/modules/BaseModule.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "utils/Component.h"
#include "utils/DebugLine.h"
#include "utils/TextColor.h"
#include "utils/Utils.h"

class ClientInstance;

namespace better_f3 {

using Json = nlohmann::ordered_json;

class BaseModule {
public:
    using Ptr = std::shared_ptr<BaseModule>;

    TextColor nameColor         = 0xFFFFFF;
    TextColor valueColor        = 0xFFFFFF;
    TextColor defaultNameColor  = 0xFFFFFF;
    TextColor defaultValueColor = 0xFFFFFF;
    bool      enabled           = true;

    std::string const id;

    static std::vector<Ptr> modules;
    static std::vector<Ptr> modulesRight;
    static std::vector<Ptr> allModules;

    explicit BaseModule(std::string id);
    virtual ~BaseModule() = default;

    static void init(Ptr const& module, PositionEnum position);
    static void init(Ptr const& module);

    [[nodiscard]] std::vector<std::unique_ptr<DebugLine>>& lines() { return mLines; }
    [[nodiscard]] std::vector<Component>                   linesFormatted(bool reducedDebug);
    [[nodiscard]] DebugLine*                               line(std::string const& lineId);

    [[nodiscard]] static Ptr moduleById(std::string const& moduleId);
    [[nodiscard]] std::string toString() const;

    void        enable(bool e);
    void        markDirty() { mDirty = true; }
    static void markAllDirty();
    [[nodiscard]] bool needsUpdate() const;
    [[nodiscard]] virtual bool updatesEveryFrame() const { return true; }
    void                       refreshDirtyFromState();
    [[nodiscard]] std::vector<Component> const& cachedLinesFormatted(bool reducedDebug);

    void loadConfig(Json const& moduleConfig);
    void saveConfig(Json& moduleConfig);

    virtual void update(ClientInstance& client) = 0;

    using Factory = std::function<Ptr()>;
    static void                          registerType(std::string const& moduleId, Factory factory);
    [[nodiscard]] static Ptr             create(std::string const& moduleId);
    [[nodiscard]] static std::vector<Ptr> distinctModules();

    static TextColor readColor(Json const& config, char const* key, TextColor defaultColor);
    static void      writeColor(Json& config, char const* key, TextColor color);

protected:
    virtual void loadModuleConfig(Json const&) {}
    virtual void saveModuleConfig(Json&) {}

    template <class T = DebugLine, class... Args>
    T& addLine(Args&&... args) {
        auto  p   = std::make_unique<T>(std::forward<Args>(args)...);
        auto& ref = *p;
        mLines.push_back(std::move(p));
        return ref;
    }

    std::vector<std::unique_ptr<DebugLine>> mLines;

private:
    [[nodiscard]] std::size_t stateHash() const;

    bool                   mDirty              = true;
    std::vector<Component> mCachedLines;
    bool                   mCachedReducedDebug = false;
    std::size_t            mCachedStateHash    = 0;
    bool                   mHasCachedStateHash = false;

    static std::map<std::string, Factory>& factories();
};

}
