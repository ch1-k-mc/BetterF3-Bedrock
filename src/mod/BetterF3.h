// SPDX-License-Identifier: MIT
#pragma once

#include "ll/api/mod/NativeMod.h"

namespace better_f3 {

class BetterF3 {

public:
    static BetterF3& getInstance();

    BetterF3() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();

    bool enable();

    bool disable();

private:
    ll::mod::NativeMod& mSelf;
};

}
