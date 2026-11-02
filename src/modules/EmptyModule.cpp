// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/modules/EmptyModule.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#include "modules/EmptyModule.h"

#include <algorithm>

namespace better_f3 {

EmptyModule::EmptyModule() : BaseModule("empty") {
    emptyLines     = defaultEmptyLines;
    displayedLines = emptyLines;
    for (int i = 0; i < 20; ++i) {
        auto& l          = addLine("nothing", "", false);
        l.inReducedDebug = true;
    }
}

BaseModule::Ptr EmptyModule::make(bool invisible) {
    auto p = std::make_shared<EmptyModule>();
    if (!invisible) allModules.push_back(p);
    return p;
}

void EmptyModule::update(ClientInstance&) {
    int const clamped   = std::min(emptyLines, 20);
    int const loopLines = clamped != displayedLines ? 20 : displayedLines;
    for (int i = 0; i < loopLines; ++i) {
        mLines[i]->value("");
        mLines[i]->active = i < clamped;
    }
    if (loopLines == 20) {
        emptyLines     = clamped;
        displayedLines = clamped;
    }
}

void EmptyModule::loadModuleConfig(Json const& moduleConfig) { emptyLines = moduleConfig.value("empty_lines", defaultEmptyLines); }

void EmptyModule::saveModuleConfig(Json& moduleConfig) { moduleConfig["empty_lines"] = emptyLines; }

}
