// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/modules/EmptyModule.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#pragma once

#include "modules/BaseModule.h"

namespace better_f3 {

class EmptyModule : public BaseModule {
public:
    int       emptyLines;
    int       displayedLines;
    int const defaultEmptyLines = 1;

    static Ptr make(bool invisible);
    EmptyModule();

    void update(ClientInstance& client) override;
    [[nodiscard]] bool updatesEveryFrame() const override { return false; }

protected:
    void loadModuleConfig(Json const& moduleConfig) override;
    void saveModuleConfig(Json& moduleConfig) override;
};

}
