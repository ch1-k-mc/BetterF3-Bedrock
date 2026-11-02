// SPDX-License-Identifier: MIT
#include "hooks/RenderStats.h"

#include "ll/api/memory/Hook.h"

#include "mc/client/renderer/game/LevelRendererCamera.h"
#include "mc/client/renderer/chunks/RenderChunkInstanced.h"
#include "mc/util/GridArea.h"

namespace better_f3::RenderStats {

namespace {
Snapshot gSnap;
bool     gHooked = false;
}

LL_TYPE_INSTANCE_HOOK(
    BetterF3LevelRendererCameraRenderHook,
    ll::memory::HookPriority::Normal,
    LevelRendererCamera,
    &LevelRendererCamera::$render,
    void,
    ::BaseActorRenderContext& baseEntityRenderContext,
    ::ViewRenderObject const& renderObj,
    ::IClientInstance&        ci
) {
    origin(baseEntityRenderContext, renderObj, ci);
    Snapshot s;
    if (auto const& area = mViewArea.get(); area) {
        for (auto const& c : area->mChunkDataBox.mContent.mChunks) {
            if (c) ++s.viewAreaChunks;
        }
    }
    s.queuedChunks = static_cast<std::size_t>(mChunkQueueSize);
    s.actorQueue   = mActorRenderQueue->keys().size();
    s.valid        = true;
    gSnap          = s;
}

bool install() {
    if (!gHooked) gHooked = BetterF3LevelRendererCameraRenderHook::hook() == 0;
    return gHooked;
}

void uninstall() {
    if (gHooked) BetterF3LevelRendererCameraRenderHook::unhook();
    gHooked = false;
}

Snapshot const& get() { return gSnap; }

}
