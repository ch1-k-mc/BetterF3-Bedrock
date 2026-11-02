// SPDX-License-Identifier: MIT
#pragma once

#include <string>

namespace better_f3::NetStats {

bool install();
void uninstall();

void countSent();

void onFrame();

struct Snapshot {
    float       avgSent         = 0;
    float       avgReceived     = 0;
    float       bytesPerSecond  = 0;
    float       pingMs          = -1;
    std::string serverVersion;
    std::string levelName;
    bool        remote          = false;
    bool        connected       = false;
    float       remoteTickMs    = 0;
};
[[nodiscard]] Snapshot get();

}
