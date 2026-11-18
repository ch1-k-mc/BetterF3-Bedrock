// SPDX-License-Identifier: MIT
#include "utils/ServerSampler.h"

#include <atomic>
#include <mutex>

#include "ll/api/service/Bedrock.h"
#include "ll/api/thread/ServerThreadExecutor.h"

#include "mc/server/ServerInstance.h"
#include "mc/server/ServerPerformanceData.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/BrightnessPair.h"
#include "mc/world/level/chunk/AverageTracker.h"
#include "mc/world/level/chunk/ChunkSource.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/ticking/TickingAreaList.h"

#include "utils/EntityCategory.h"
#include "utils/Utils.h"

namespace better_f3::ServerSampler {

namespace {

std::mutex          gMu;
Snapshot            gSnap;
std::atomic<bool>   gPending{false};
std::int64_t        gLastRequest = 0;
std::atomic<int>    gGeneration{0};

std::size_t countForceloaded(Dimension&) { return 0; }

Snapshot sample() {
    Snapshot s;
    auto     level = ll::service::getLevel();
    if (!level) return s;
    Player* player = nullptr;
    level->forEachPlayer([&](Player& p) {
        player = &p;
        return false;
    });
    if (!player) return s;
    BlockPos const   pos{player->getFeetPos()};
    BlockSource&     bs = player->getDimensionBlockSource();
    BrightnessPair const br = bs.getBrightnessPair(pos);
    s.sky   = br.sky->mValue;
    s.block = br.block->mValue;
    s.heights = {
        {"S",  bs.getHeightmapPos(pos).y                                },
        {"O",  static_cast<int>(bs.getAboveTopSolidBlock(pos.x, pos.z, false, false))},
        {"M",  static_cast<int>(bs.getAboveTopSolidBlock(pos.x, pos.z, true, true))  },
        {"ML", static_cast<int>(bs.getAboveTopSolidBlock(pos.x, pos.z, true, false)) },
    };
    Dimension& dim      = player->getDimension();
    if (auto* cs = dim.mChunkSource.get()) s.loadedChunks = cs->getStorage().size();
    s.forceloadedChunks = countForceloaded(dim);
    s.time              = level->getTime();
    if (auto si = ll::service::getServerInstance()) {
        if (auto* perf = si->mServerPerformanceData.get()) s.tickMs = perf->mServerGameTickTime->mAverageOverLastSecond * 1000.0f;
    }
    for (auto* a : level->getRuntimeActorList()) {
        if (!a) continue;
        ++s.categories[entityCategoryName(*a)];
    }
    s.valid = true;
    return s;
}

}

void request() {
    if (!ll::service::getServerInstance()) return;
    std::int64_t const now = utils::millis();
    if (gPending.load() || now - gLastRequest < 250) return;
    gLastRequest = now;
    gPending     = true;
    int const gen = gGeneration.load();
    ll::thread::ServerThreadExecutor::getDefault().execute([gen] {
        Snapshot s = sample();
        if (gen == gGeneration.load()) {
            std::lock_guard lk(gMu);
            gSnap = std::move(s);
        }
        gPending = false;
    });
}

Snapshot get() {
    std::lock_guard lk(gMu);
    return gSnap;
}

void clear() {
    ++gGeneration;
    std::lock_guard lk(gMu);
    gSnap = Snapshot{};
}

}
