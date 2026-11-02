// SPDX-License-Identifier: MIT
#include "render/WorldLines.h"

#include <cmath>

#include "ll/api/memory/Hook.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/screens/ScreenContext.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/client/renderer/BaseActorRenderContext.h"
#include "mc/client/renderer/Tessellator.h"
#include "mc/client/renderer/game/LevelRendererCamera.h"
#include "mc/common/client/renderer/helpers/MeshHelpers.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core_graphics/enums/PrimitiveMode.h"
#include "mc/deps/minecraft_renderer/renderer/MaterialPtr.h"
#include "mc/deps/minecraft_renderer/resources/OffscreenCaptureDescription.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/phys/AABB.h"

#include "input/F3Combos.h"

namespace better_f3::WorldLines {

namespace {

bool               gHooked = false;
unsigned long long gLines  = 0;

struct Lines {
    ::Tessellator&     t;
    Vec3               cam;
    unsigned long long count = 0;

    void color(std::uint32_t argb) {
        t.color(
            static_cast<float>((argb >> 16) & 0xFF) / 255.f,
            static_cast<float>((argb >> 8) & 0xFF) / 255.f,
            static_cast<float>(argb & 0xFF) / 255.f,
            static_cast<float>((argb >> 24) & 0xFF) / 255.f
        );
    }
    void line(float ax, float ay, float az, float bx, float by, float bz) {
        t.vertex(ax - cam.x, ay - cam.y, az - cam.z);
        t.vertex(bx - cam.x, by - cam.y, bz - cam.z);
        ++count;
    }
    void box(Vec3 const& a, Vec3 const& b) {
        line(a.x, a.y, a.z, b.x, a.y, a.z); line(b.x, a.y, a.z, b.x, a.y, b.z); line(b.x, a.y, b.z, a.x, a.y, b.z); line(a.x, a.y, b.z, a.x, a.y, a.z);
        line(a.x, b.y, a.z, b.x, b.y, a.z); line(b.x, b.y, a.z, b.x, b.y, b.z); line(b.x, b.y, b.z, a.x, b.y, b.z); line(a.x, b.y, b.z, a.x, b.y, a.z);
        line(a.x, a.y, a.z, a.x, b.y, a.z); line(b.x, a.y, a.z, b.x, b.y, a.z); line(b.x, a.y, b.z, b.x, b.y, b.z); line(a.x, a.y, b.z, a.x, b.y, b.z);
    }
};

void hitboxes(Lines& l, LocalPlayer& self) {
    for (Actor* a : self.getLevel().getRuntimeActorList()) {
        if (!a || a == &self) continue;
        AABB const& bb = a->getAABB();
        l.color(0xFFFFFFFF);
        l.box(bb.min, bb.max);
        Vec3 const eye = a->getEyePos();
        if (eye.y > bb.min.y && eye.y < bb.max.y) {
            l.color(0xFFFF0000);
            float const e = 0.01f;
            l.line(bb.min.x - e, eye.y, bb.min.z - e, bb.max.x + e, eye.y, bb.min.z - e);
            l.line(bb.max.x + e, eye.y, bb.min.z - e, bb.max.x + e, eye.y, bb.max.z + e);
            l.line(bb.max.x + e, eye.y, bb.max.z + e, bb.min.x - e, eye.y, bb.max.z + e);
            l.line(bb.min.x - e, eye.y, bb.max.z + e, bb.min.x - e, eye.y, bb.min.z - e);
        }
        Vec3 const v = a->getViewVector(1.0f);
        l.color(0xFF0000FF);
        l.line(eye.x, eye.y, eye.z, eye.x + v.x * 2.f, eye.y + v.y * 2.f, eye.z + v.z * 2.f);
    }
}

void chunkBorders(Lines& l, LocalPlayer& self) {
    Vec3 const  pos  = self.getFeetPos();
    int const   cx   = static_cast<int>(std::floor(pos.x)) >> 4;
    int const   cz   = static_cast<int>(std::floor(pos.z)) >> 4;
    float const x0   = static_cast<float>(cx * 16);
    float const z0   = static_cast<float>(cz * 16);
    auto const& h    = self.getDimension().mHeightRange;
    float const minY = static_cast<float>(static_cast<short>(h->mMin));
    float const maxY = static_cast<float>(static_cast<short>(h->mMax));
    l.color(0xFFFF0000);
    for (int dx = -16; dx <= 32; dx += 16) {
        for (int dz = -16; dz <= 32; dz += 16) {
            if ((dx == 0 || dx == 16) && (dz == 0 || dz == 16)) continue;
            l.line(x0 + static_cast<float>(dx), minY, z0 + static_cast<float>(dz), x0 + static_cast<float>(dx), maxY, z0 + static_cast<float>(dz));
        }
    }
    for (int i = 0; i <= 16; i += 2) {
        float const f      = static_cast<float>(i);
        bool const  corner = i == 0 || i == 16;
        l.color(corner ? 0xFF00FFFF : 0xFFFFFF00);
        l.line(x0 + f, minY, z0, x0 + f, maxY, z0);
        l.line(x0 + f, minY, z0 + 16.f, x0 + f, maxY, z0 + 16.f);
        l.line(x0, minY, z0 + f, x0, maxY, z0 + f);
        l.line(x0 + 16.f, minY, z0 + f, x0 + 16.f, maxY, z0 + f);
    }
    for (float y = minY; y <= maxY; y += 2.f) {
        bool const section = static_cast<int>(y) % 16 == 0;
        l.color(section ? 0xFF3F3FFF : 0xFFFFFF00);
        l.line(x0, y, z0, x0 + 16.f, y, z0);
        l.line(x0 + 16.f, y, z0, x0 + 16.f, y, z0 + 16.f);
        l.line(x0 + 16.f, y, z0 + 16.f, x0, y, z0 + 16.f);
        l.line(x0, y, z0 + 16.f, x0, y, z0);
    }
}

}

LL_TYPE_INSTANCE_HOOK(
    BetterF3WorldLinesHook,
    ll::memory::HookPriority::Normal,
    LevelRendererCamera,
    &LevelRendererCamera::$renderBlockEntities,
    void,
    ::BaseActorRenderContext& renderContext,
    bool                      renderAlphaLayer
) {
    origin(renderContext, renderAlphaLayer);
    if (!renderAlphaLayer) return;
    bool const hb = F3Combos::hitboxes();
    bool const cb = F3Combos::chunkBorders();
    if (!hb && !cb) {
        gLines = 0;
        return;
    }
    auto  ci     = ll::service::getClientInstance();
    auto* player = ci ? ci->getLocalPlayer() : nullptr;
    if (!player) return;
    ::ScreenContext& sc = renderContext.mScreenContext;
    ::Tessellator&   t  = sc.tessellator;
    Vec3 const       saved = t.mPostTransformOffset;
    t.begin(::Tessellator::DebugContextCallback{}, ::mce::PrimitiveMode::LineList, 0, false);
    t.mPostTransformOffset = Vec3{0, 0, 0};
    Lines l{t, mCameraPos};
    if (hb) hitboxes(l, *player);
    if (cb) chunkBorders(l, *player);
    gLines = l.count;
    if (l.count > 0) ::MeshHelpers::renderMeshImmediately(sc, t, wireframeMaterial, ::OffscreenCaptureDescription{});
    else t.clear();
    t.mPostTransformOffset = saved;
}

bool install() {
    if (!gHooked) gHooked = BetterF3WorldLinesHook::hook() == 0;
    return gHooked;
}

void uninstall() {
    if (gHooked) BetterF3WorldLinesHook::unhook();
    gHooked = false;
}

unsigned long long lastLineCount() { return gLines; }

}
