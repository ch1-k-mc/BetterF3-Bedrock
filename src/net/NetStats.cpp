// SPDX-License-Identifier: MIT
#include "net/NetStats.h"

#include <atomic>
#include <mutex>

#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/game/MinecraftGame.h"
#include "mc/client/network/ClientNetworkHandler.h"
#include "mc/client/network/LegacyClientNetworkHandler.h"
#include "mc/network/ClientNetworkSystem.h"
#include "mc/network/IncomingPacketFilterResult.h"
#include "mc/network/MinecraftPacketIds.h"
#include "mc/network/NetworkConnection.h"
#include "mc/network/NetworkIdentifierWithSubId.h"
#include "mc/network/NetworkPeer.h"
#include "mc/network/packet/StartGamePacket.h"
#include "mc/network/packet/StartGamePacketPayload.h"
#include "mc/profile/ProfilerLite.h"

#include "graphs/DebugCharts.h"
#include "utils/Utils.h"

namespace better_f3::NetStats {

namespace {

std::atomic<std::uint64_t> gSent{0};
std::atomic<std::uint64_t> gReceived{0};
std::atomic<std::uint64_t> gReceivedBytes{0};
std::mutex                 gMu;
std::string                gServerVersion;
std::string                gLevelName;
bool                       gHooked = false;

std::uint64_t gLastSent = 0, gLastReceived = 0, gLastBytes = 0;
std::int64_t  gLastSecond = 0;
float         gAvgSent = 0, gAvgReceived = 0, gBytesPerSecond = 0, gPing = -1;
float         gLastRemoteTick = 0;

float lerp(float t, float a, float b) { return a + t * (b - a); }

}

LL_TYPE_INSTANCE_HOOK(
    BetterF3IncomingPacketHook,
    ll::memory::HookPriority::Normal,
    ClientNetworkHandler,
    &ClientNetworkHandler::$allowIncomingPacketId,
    ::IncomingPacketFilterResult,
    ::NetworkIdentifierWithSubId const& id,
    ::MinecraftPacketIds                packetId,
    uint64                              packetSize
) {
    ++gReceived;
    gReceivedBytes += packetSize;
    return origin(id, packetId, packetSize);
}

LL_TYPE_INSTANCE_HOOK(
    BetterF3StartGameHook,
    ll::memory::HookPriority::Normal,
    LegacyClientNetworkHandler,
    &LegacyClientNetworkHandler::$handle,
    void,
    ::NetworkIdentifier const& source,
    ::StartGamePacket const&   packet
) {
    {
        std::lock_guard lk(gMu);
        gServerVersion = packet.mServerVersion;
        gLevelName     = packet.mLevelName;
    }
    origin(source, packet);
}

bool install() {
    if (gHooked) return true;
    bool ok = BetterF3IncomingPacketHook::hook() == 0;
    ok      = (BetterF3StartGameHook::hook() == 0) && ok;
    gHooked = true;
    return ok;
}

void uninstall() {
    if (!gHooked) return;
    BetterF3IncomingPacketHook::unhook();
    BetterF3StartGameHook::unhook();
    gHooked = false;
}

void countSent() { ++gSent; }

void onFrame() {
    std::int64_t const now = utils::millis();
    float const remoteTick = ProfilerLite::gProfilerLiteInstance().mDebugRemoteServerTickTime;
    if (remoteTick > 0 && remoteTick != gLastRemoteTick) DebugCharts::pushTick(remoteTick);
    gLastRemoteTick = remoteTick;
    if (now - gLastSecond < 1000) return;
    float const dt = gLastSecond == 0 ? 1.f : static_cast<float>(now - gLastSecond) / 1000.f;
    gLastSecond    = now;
    std::uint64_t const s = gSent.load(), r = gReceived.load(), b = gReceivedBytes.load();
    float const         sentPerSec = static_cast<float>(s - gLastSent) / dt;
    float const         recvPerSec = static_cast<float>(r - gLastReceived) / dt;
    gBytesPerSecond                = static_cast<float>(b - gLastBytes) / dt;
    gLastSent = s, gLastReceived = r, gLastBytes = b;
    gAvgSent     = lerp(0.75f, sentPerSec, gAvgSent);
    gAvgReceived = lerp(0.75f, recvPerSec, gAvgReceived);

    gPing = -1;
    if (auto ci = ll::service::getClientInstance(); ci && ci->getLocalPlayer()) {
        auto& mg  = static_cast<MinecraftGame&>(ci->getMinecraftGame());
        auto& sys = mg.getClientNetworkSystem();
        std::lock_guard lk(*sys.mConnectionsMutex);
        for (auto const& c : *sys.mConnections) {
            if (!c) continue;
            auto peer = c->mPeer.get();
            if (!peer) continue;
            auto status = peer->getNetworkStatus();
            gPing       = static_cast<float>(status.mAveragePing->count());
            break;
        }
    }
    DebugCharts::pushPing(gPing < 0 ? 0.f : gPing);
    DebugCharts::pushBandwidth(gBytesPerSecond);
}

Snapshot get() {
    Snapshot s;
    s.avgSent        = gAvgSent;
    s.avgReceived    = gAvgReceived;
    s.bytesPerSecond = gBytesPerSecond;
    s.pingMs         = gPing;
    s.remoteTickMs   = gLastRemoteTick;
    s.remote         = !ll::service::getServerInstance();
    auto ci          = ll::service::getClientInstance();
    s.connected      = ci && ci->getLocalPlayer();
    std::lock_guard lk(gMu);
    s.serverVersion = gServerVersion;
    s.levelName     = gLevelName;
    return s;
}

}
