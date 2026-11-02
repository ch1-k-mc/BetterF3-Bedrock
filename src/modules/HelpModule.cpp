// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/modules/HelpModule.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#include "modules/HelpModule.h"

#include "utils/DebugOverlay.h"
#include "utils/Lang.h"

namespace better_f3 {

HelpModule::HelpModule() : BaseModule("help") {
    defaultNameColor  = 0xFDFD96;
    defaultValueColor = legacyColor(ChatFormatting::AQUA);
    nameColor         = defaultNameColor;
    valueColor        = defaultValueColor;
    enabledColor      = defaultEnabledColor;
    disabledColor     = defaultDisabledColor;

    addLine("pie_graph_new");
    addLine("fps_tps_new");
    addLine("ping");
    addLine("help");
    for (auto& l : mLines) l->inReducedDebug = true;
}

void HelpModule::update(ClientInstance&) {
    std::string const visible = Lang::get().get("text.betterf3.line.visible");
    std::string const hidden  = Lang::get().get("text.betterf3.line.hidden");
    auto state = [&](bool on) { return on ? utils::styledText(visible, enabledColor) : utils::styledText(hidden, disabledColor); };
    mLines[0]->value(state(DebugOverlay::renderProfilerChart));
    mLines[1]->value(state(DebugOverlay::renderFpsCharts));
    mLines[2]->value(state(DebugOverlay::renderNetworkCharts));
    mLines[3]->value(Lang::get().get("text.betterf3.line.help_press"));
}

void HelpModule::loadModuleConfig(Json const& moduleConfig) {
    enabledColor  = readColor(moduleConfig, "enabled_color", defaultEnabledColor);
    disabledColor = readColor(moduleConfig, "disabled_color", defaultDisabledColor);
}

void HelpModule::saveModuleConfig(Json& moduleConfig) {
    writeColor(moduleConfig, "enabled_color", enabledColor);
    writeColor(moduleConfig, "disabled_color", disabledColor);
}

}
