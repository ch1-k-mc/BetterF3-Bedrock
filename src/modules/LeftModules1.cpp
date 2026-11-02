// SPDX-License-Identifier: MIT
#include "modules/LeftModules1.h"

#include <algorithm>
#include <cmath>
#include <random>

#include "ll/api/Versions.h"
#include "ll/api/service/Bedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/options/IOptionRegistry.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/options/GraphicsMode.h"
#include "mc/profile/ProfilerLite.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/Tick.h"
#include "mc/world/level/biome/Biome.h"
#include "mc/world/level/block/BrightnessPair.h"
#include "mc/world/level/chunk/ChunkSource.h"
#include "mc/world/level/chunk/LevelChunk.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/VanillaDimensions.h"

#include "hooks/RenderStats.h"
#include "net/NetStats.h"
#include "utils/Lang.h"
#include "utils/ServerSampler.h"

namespace better_f3 {

namespace {

std::string tr(std::string const& key) { return Lang::get().get(key); }

int currentFps() { return static_cast<int>(std::lround(ProfilerLite::gProfilerLiteInstance().mFrameData->mFPS)); }

std::string trim(std::string s) {
    while (!s.empty() && s.back() == ' ') s.pop_back();
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    return s;
}

std::string toStr(long long v) { return std::to_string(v); }

bool isSlimeChunk(int cx, int cz) {
    std::uint32_t const seed = static_cast<std::uint32_t>(cx) * 0x1f1f1f1fu ^ static_cast<std::uint32_t>(cz);
    std::mt19937        mt(seed);
    return mt() % 10 == 0;
}

std::string heights(BlockSource& bs, BlockPos const& pos, bool server) {
    std::vector<std::pair<std::string, int>> list{{"S", bs.getHeightmapPos(pos).y}};
    if (server) list.emplace_back("O", static_cast<int>(bs.getAboveTopSolidBlock(pos.x, pos.z, false, false)));
    list.emplace_back("M", static_cast<int>(bs.getAboveTopSolidBlock(pos.x, pos.z, true, true)));
    list.emplace_back("ML", static_cast<int>(bs.getAboveTopSolidBlock(pos.x, pos.z, true, false)));
    std::string out;
    for (auto& [n, y] : list) out += "  " + n + ": " + std::to_string(y);
    return trim(out);
}

}

MinecraftModule::MinecraftModule() : BaseModule("minecraft") {
    defaultNameColor  = 0xA0522D;
    defaultValueColor = legacyColor(ChatFormatting::DARK_GREEN);
    nameColor         = defaultNameColor;
    valueColor        = defaultValueColor;
    addLine("minecraft", "format.betterf3.default_no_colon", false).inReducedDebug = true;
}

void MinecraftModule::update(ClientInstance&) {
    auto const game = ll::getGameVersion();
    std::string name = std::to_string(game.major) + "." + std::to_string(game.minor) + "." + std::to_string(game.patch);
    mLines[0]->value(name + " (" + ll::getLoaderVersion().to_string() + "/LeviLamina)");
}

FpsModule::FpsModule() : BaseModule("fps") {
    addLine("fps", "format.betterf3.no_format", true).inReducedDebug = true;
    colorHigh = defaultColorHigh;
    colorMed  = defaultColorMed;
    colorLow  = defaultColorLow;
}

void FpsModule::update(ClientInstance& client) {
    int const fps   = currentFps();
    int const limit = client.getOptions().getMaxFrameRate();
    std::string const fpsString = trim(utils::i18n(
        "format.betterf3.fps",
        {std::to_string(fps), limit <= 0 ? tr("text.betterf3.line.fps.unlimited") : std::to_string(limit), ""}
    ));
    TextColor color = colorLow;
    switch (utils::fpsColor(fps)) {
    case FpsEnum::HIGH: color = colorHigh; break;
    case FpsEnum::MEDIUM: color = colorMed; break;
    case FpsEnum::LOW: color = colorLow; break;
    }
    mLines[0]->value(std::vector<Component>{utils::styledText(fpsString, color)});
}

void FpsModule::loadModuleConfig(Json const& c) {
    colorHigh = readColor(c, "color_high", defaultColorHigh);
    colorMed  = readColor(c, "color_med", defaultColorMed);
    colorLow  = readColor(c, "color_low", defaultColorLow);
}

void FpsModule::saveModuleConfig(Json& c) {
    writeColor(c, "color_high", colorHigh);
    writeColor(c, "color_med", colorMed);
    writeColor(c, "color_low", colorLow);
}

GraphicsModule::GraphicsModule() : BaseModule("graphics") {
    defaultNameColor  = legacyColor(ChatFormatting::GOLD);
    defaultValueColor = legacyColor(ChatFormatting::AQUA);
    nameColor         = defaultNameColor;
    valueColor        = defaultValueColor;
    addLine("render_distance");
    addLine("graphics");
    addLine("clouds");
    addLine("biome_blend_radius").active = false;
    addLine("shader").active             = false;
}

void GraphicsModule::update(ClientInstance& client) {
    auto& o = client.getOptions();
    mLines[0]->value(std::to_string(o.getViewDistanceChunks()));
    std::string mode;
    switch (o.getGraphicsMode()) {
    case GraphicsMode::Simple: mode = "Simple"; break;
    case GraphicsMode::Fancy: mode = "Fancy"; break;
    case GraphicsMode::Advanced: mode = "Vibrant Visuals"; break;
    case GraphicsMode::RayTraced: mode = "Ray Traced"; break;
    default: mode = std::to_string(static_cast<int>(o.getGraphicsMode())); break;
    }
    mLines[1]->value(mode);
    mLines[2]->value(o.getRenderClouds() ? tr("text.betterf3.line.fancy") : tr("text.betterf3.line.off"));
    mLines[0]->inReducedDebug = true;
    mLines[3]->inReducedDebug = true;
}

ServerModule::ServerModule() : BaseModule("server") {
    defaultNameColor  = legacyColor(ChatFormatting::GRAY);
    defaultValueColor = legacyColor(ChatFormatting::YELLOW);
    nameColor         = defaultNameColor;
    valueColor        = defaultValueColor;
    addLine("server_tick", "format.betterf3.server_tick", true);
    addLine("packets_sent");
    addLine("packets_received");
    addLine("tick_manager_status").active = false;
    for (auto& l : mLines) l->inReducedDebug = true;
}

void ServerModule::update(ClientInstance&) {
    bool const  integrated = static_cast<bool>(ll::service::getServerInstance());
    auto const  net        = NetStats::get();
    std::string serverString;
    std::string tickString;
    if (integrated) {
        serverString = tr("text.betterf3.line.integrated_server");
        auto snap    = ServerSampler::get();
        if (snap.valid) tickString = std::to_string(std::lround(snap.tickMs));
    } else if (net.connected) {
        serverString = net.serverVersion.empty() ? std::string("Bedrock Server") : "Bedrock Server " + net.serverVersion;
        if (net.remoteTickMs > 0) tickString = std::to_string(std::lround(net.remoteTickMs));
    }
    std::vector<Component> list{utils::styledText(serverString, nameColor), utils::styledText(tickString, nameColor)};
    if (tickString.empty()) {
        mLines[0]->format("format.betterf3.no_format");
        list.pop_back();
    } else {
        mLines[0]->format("format.betterf3.server_tick");
    }
    mLines[0]->value(list);
    if (net.connected) {
        mLines[1]->value(std::to_string(std::lround(net.avgSent)));
        mLines[2]->value(std::to_string(std::lround(net.avgReceived)));
    }
}

CoordsModule::CoordsModule() : BaseModule("coords") {
    defaultNameColor = legacyColor(ChatFormatting::RED);
    nameColor        = defaultNameColor;
    colorX           = defaultColorX;
    colorY           = defaultColorY;
    colorZ           = defaultColorZ;
    addLine("player_coords", FORMAT_COORDS, true);
    addLine("block_coords", FORMAT_COORDS, true);
    addLine("chunk_relative_coords", FORMAT_COORDS, true);
    addLine("chunk_coords", FORMAT_COORDS, true);
    addLine("velocity", FORMAT_COORDS, true);
    addLine("absolute_velocity");
    addLine("horizontal_velocity");
    mLines[2]->inReducedDebug = true;
}

void CoordsModule::update(ClientInstance& client) {
    auto* player = client.getLocalPlayer();
    if (!player) return;
    Component xyz = utils::styledText("X", colorX);
    xyz.append(utils::styledText("Y", colorY)).append(utils::styledText("Z", colorZ));

    Vec3 const pos = player->getFeetPos();
    mLines[0]->value(std::vector<Component>{
        xyz,
        utils::styledText(utils::fixed(pos.x, 3), colorX),
        utils::styledText(utils::fixed(pos.y, 5), colorY),
        utils::styledText(utils::fixed(pos.z, 3), colorZ)});
    BlockPos const bp{pos};
    auto three = [&](long long x, long long y, long long z) {
        return std::vector<Component>{utils::styledText(toStr(x), colorX), utils::styledText(toStr(y), colorY), utils::styledText(toStr(z), colorZ)};
    };
    mLines[1]->value(three(bp.x, bp.y, bp.z));
    mLines[2]->value(three(bp.x & 15, bp.y & 15, bp.z & 15));
    mLines[3]->value(three(bp.x >> 4, bp.y >> 4, bp.z >> 4));

    Actor* vehicle = player;
    for (int guard = 0; guard < 16; ++guard) {
        Actor* v = vehicle->getVehicle();
        if (!v) break;
        vehicle = v;
    }
    int const     mult    = std::min(currentFps(), 20);
    Vec3 const    current = vehicle->getFeetPos();
    std::uint64_t now     = player->getLevel().getCurrentTick().tickID;
    if (!(mPrevPos == current)) {
        mVelocity           = mPrevPos - current;
        mPositionUpdateTime = now;
    } else if (now - mPositionUpdateTime > 1) {
        mVelocity = Vec3{0, 0, 0};
    }
    mPrevPos = current;
    mLines[4]->value(std::vector<Component>{
        utils::styledText(utils::fixed(mVelocity.x * mult, 3), colorX),
        utils::styledText(utils::fixed(mVelocity.y * mult, 3), colorY),
        utils::styledText(utils::fixed(mVelocity.z * mult, 3), colorZ)});
    double const len  = std::sqrt(mVelocity.x * mVelocity.x + mVelocity.y * mVelocity.y + mVelocity.z * mVelocity.z);
    double const hlen = std::sqrt(mVelocity.x * mVelocity.x + mVelocity.z * mVelocity.z);
    mLines[5]->value(utils::styledText(utils::fixed(len * mult, 3), defaultNameColor));
    mLines[6]->value(utils::styledText(utils::fixed(hlen * mult, 3), defaultNameColor));
}

void CoordsModule::loadModuleConfig(Json const& c) {
    colorX = readColor(c, "color_x", defaultColorX);
    colorY = readColor(c, "color_y", defaultColorY);
    colorZ = readColor(c, "color_z", defaultColorZ);
}

void CoordsModule::saveModuleConfig(Json& c) {
    writeColor(c, "color_x", colorX);
    writeColor(c, "color_y", colorY);
    writeColor(c, "color_z", colorZ);
}

ChunksModule::ChunksModule() : BaseModule("chunks") {
    defaultNameColor  = 0x00AAFF;
    defaultValueColor = legacyColor(ChatFormatting::YELLOW);
    nameColor         = defaultNameColor;
    valueColor        = defaultValueColor;
    totalColor        = defaultTotalColor;
    enabledColor      = defaultEnabledColor;
    disabledColor     = defaultDisabledColor;
    addLine("chunk_sections", "format.betterf3.total", true);
    addLine("chunk_culling").active     = false;
    addLine("pending_chunks").active    = false;
    addLine("pending_uploads").active   = false;
    addLine("available_buffers").active = false;
    addLine("client_chunk_cache");
    addLine("loaded_chunks");
    addLine("loaded_chunks_server");
    addLine("forceloaded_chunks");
    addLine("spawn_chunks").active = false;
    addLine("chunk_file").active   = false;
    for (int i : {0, 2, 3, 4, 5}) mLines[i]->inReducedDebug = true;
}

void ChunksModule::update(ClientInstance& client) {
    auto* player = client.getLocalPlayer();
    if (!player) return;
    auto const& rs = RenderStats::get();
    mLines[0]->value(std::vector<Component>{
        utils::styledText(tr("text.betterf3.line.rendered"), valueColor),
        utils::styledText(tr("text.betterf3.line.total"), totalColor),
        utils::styledText(std::to_string(rs.queuedChunks), valueColor),
        utils::styledText(std::to_string(rs.viewAreaChunks), totalColor)});

    auto const& storage = player->getDimension().getChunkSource().getStorage();
    std::size_t alive   = 0;
    for (auto const& [pos, weak] : storage) {
        if (!weak.expired()) ++alive;
    }
    mLines[5]->value(std::to_string(storage.size()));
    mLines[6]->value(std::to_string(alive));

    auto const snap = ServerSampler::get();
    if (snap.valid) mLines[7]->value(std::to_string(snap.loadedChunks));
    else mLines[7]->active = false;
    mLines[8]->value(std::to_string(snap.valid ? snap.forceloadedChunks : 0));
}

void ChunksModule::loadModuleConfig(Json const& c) {
    totalColor    = readColor(c, "total_chunks_color", defaultTotalColor);
    enabledColor  = readColor(c, "chunks_enabled_color", defaultEnabledColor);
    disabledColor = readColor(c, "chunks_disabled_color", defaultDisabledColor);
}

void ChunksModule::saveModuleConfig(Json& c) {
    writeColor(c, "chunks_enabled_color", enabledColor);
    writeColor(c, "chunks_disabled_color", disabledColor);
    writeColor(c, "total_chunks_color", totalColor);
}

LocationModule::LocationModule() : BaseModule("location") {
    defaultNameColor  = legacyColor(ChatFormatting::DARK_GREEN);
    defaultValueColor = legacyColor(ChatFormatting::AQUA);
    nameColor         = defaultNameColor;
    valueColor        = defaultValueColor;
    for (char const* id : {"dimension", "facing", "rotation", "light", "light_server", "highest_block", "highest_block_server", "biome", "local_difficulty", "day_ticks", "days_played", "slime_chunk"}) {
        addLine(id);
    }
    mLines[8]->active = false;
}

void LocationModule::update(ClientInstance& client) {
    auto* player = client.getLocalPlayer();
    if (!player) return;
    Level&         level = player->getLevel();
    BlockSource&   bs    = player->getDimensionBlockSource();
    BlockPos const pos{player->getFeetPos()};

    std::string biome = bs.getBiome(pos).mHash->getString();
    if (biome.find(':') == std::string::npos) biome = "minecraft:" + biome;
    mLines[7]->value(biome);

    BrightnessPair const br    = bs.getBrightnessPair(pos);
    int const            sky   = br.sky->mValue;
    int const            block = br.block->mValue;
    mLines[3]->value(utils::i18n("format.betterf3.chunklight", {std::to_string(std::max(sky, block)), std::to_string(sky), std::to_string(block)}));

    auto const snap = ServerSampler::get();
    int const ssky = snap.valid ? snap.sky : sky;
    int const sblk = snap.valid ? snap.block : block;
    mLines[4]->value(utils::i18n("format.betterf3.chunklight_server", {std::to_string(ssky), std::to_string(sblk)}));
    mLines[5]->value(heights(bs, pos, false));
    if (snap.valid) {
        std::string s;
        for (auto& [n, y] : snap.heights) s += "  " + n + ": " + std::to_string(y);
        mLines[6]->value(trim(s));
    } else {
        mLines[6]->active = false;
    }

    std::string dim = VanillaDimensions::toString(player->getDimensionId());
    std::transform(dim.begin(), dim.end(), dim.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (dim.find(':') == std::string::npos) dim = "minecraft:" + dim;
    mLines[0]->value(dim);

    Vec2 const rot  = player->getRotation();
    int const  dir2 = static_cast<int>(std::floor(rot.y / 90.0 + 0.5)) & 3;
    Direction  facing = Direction::SOUTH;
    char const* name  = "south";
    switch (dir2) {
    case 0: facing = Direction::SOUTH; name = "south"; break;
    case 1: facing = Direction::WEST; name = "west"; break;
    case 2: facing = Direction::NORTH; name = "north"; break;
    case 3: facing = Direction::EAST; name = "east"; break;
    }
    mLines[1]->value(utils::javaFormat("%s (%s)", {tr(std::string("text.betterf3.line.") + name), utils::facingString(facing)}));

    mLines[2]->value(utils::i18n("format.betterf3.rotation", {utils::fixed(utils::wrapDegrees(rot.y), 1), utils::fixed(utils::wrapDegrees(rot.x), 1)}));

    long long const time = level.getTime();
    mLines[9]->value(std::to_string(time % 24000LL));
    mLines[10]->value(std::to_string(time / 24000LL));

    mLines[11]->value(tr(isSlimeChunk(pos.x >> 4, pos.z >> 4) ? "text.betterf3.line.slime_chunk.true" : "text.betterf3.line.slime_chunk.false"));
}

}
