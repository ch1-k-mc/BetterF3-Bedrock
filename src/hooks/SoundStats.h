// SPDX-License-Identifier: MIT
#pragma once

namespace better_f3::SoundStats {

struct Snapshot {
    bool valid           = false;
    int  playing         = 0;
    int  streaming       = 0;
    int  maxChannels     = 0;
};

bool install();
void uninstall();
[[nodiscard]] Snapshot get();

}
