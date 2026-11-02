// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/modules/HelpModule.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#pragma once

#include "modules/BaseModule.h"

namespace better_f3 {

class HelpModule : public BaseModule {
public:
    TextColor const defaultEnabledColor  = legacyColor(ChatFormatting::GREEN);
    TextColor const defaultDisabledColor = legacyColor(ChatFormatting::RED);
    TextColor       enabledColor;
    TextColor       disabledColor;

    HelpModule();
    void update(ClientInstance& client) override;

protected:
    void loadModuleConfig(Json const& moduleConfig) override;
    void saveModuleConfig(Json& moduleConfig) override;
};

}
