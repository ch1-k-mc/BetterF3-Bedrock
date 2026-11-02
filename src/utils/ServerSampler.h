// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace better_f3::ServerSampler {

struct Snapshot {
    bool         valid = false;
    int          sky   = 0;
    int          block = 0;
    std::vector<std::pair<std::string, int>> heights;
    std::size_t  loadedChunks      = 0;
    std::size_t  forceloadedChunks = 0;
    float        tickMs            = 0;
    std::int64_t time              = 0;
    std::map<std::string, int> categories;
};

void request();
[[nodiscard]] Snapshot get();
void clear();

}
