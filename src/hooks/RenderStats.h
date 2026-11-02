// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>

namespace better_f3::RenderStats {

struct Snapshot {
    std::size_t viewAreaChunks = 0;
    std::size_t queuedChunks   = 0;
    std::size_t actorQueue     = 0;
    bool        valid          = false;
};

bool install();
void uninstall();
[[nodiscard]] Snapshot const& get();

}
