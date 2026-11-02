// SPDX-License-Identifier: MIT
#include "modules/Modules2.h"

#include <cmath>
#include <cstdio>
#include <map>

#include "ll/api/service/Bedrock.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/particlesystem/particle/ParticleSystemEngine.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/client/renderer/game/LevelRenderer.h"
#include "mc/deps/application/AppPlatform.h"
#include "mc/deps/nbt/ByteTag.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/deps/nbt/StringTag.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/ShapeType.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/BlockType.h"
#include "mc/world/phys/HitResult.h"

#include "api/PublicApi.h"
#include "hooks/RenderStats.h"
#include "hooks/SoundStats.h"
#include "utils/EntityCategory.h"
#include "utils/Lang.h"
#include "utils/SystemInfo.h"

namespace better_f3 {

namespace {

std::string tr(std::string const& key) { return Lang::get().get(key); }

std::string posString(BlockPos const& p) { return std::to_string(p.x) + ", " + std::to_string(p.y) + ", " + std::to_string(p.z); }

std::string spaceD(long long v, int width) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "% *lld", width, v);
    return buf;
}
std::string zeroD(long long v, int width) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%0*lld", width, v);
    return buf;
}

std::vector<std::string> blockStates(Block const& b) {
    std::vector<std::string> out;
    auto const&              root = *b.mSerializationId;
    if (!root.contains("states")) return out;
    auto const& states = root.at("states");
    if (!states.is_object()) return out;
    for (auto const& [name, v] : states.items()) {
        std::string value;
        if (v.is_boolean()) value = v.get<ByteTag>().data != 0 ? "true" : "false";
        else if (v.is_string()) value = static_cast<std::string const&>(v.get<StringTag>());
        else value = v.toSnbt(SnbtFormat::Minimize);
        out.push_back(utils::propertyToString(name, value));
    }
    return out;
}

std::vector<std::string> blockTags(Block const& b) {
    std::vector<std::string> out;
    auto add = [&](std::vector<HashedString> const& tags) {
        for (auto const& t : tags) {
            std::string s = t.getString();
            if (s.find(':') == std::string::npos) s = "minecraft:" + s;
            std::string full = "#" + s;
            if (std::find(out.begin(), out.end(), full) == out.end()) out.push_back(full);
        }
    };
    add(*b.mTags);
    add(*b.getBlockType().mTags);
    return out;
}

HitResult pick(LocalPlayer& player, bool liquid) {
    Vec3 const  eye   = player.getPosition();
    Vec2 const  rot   = player.getRotation();
    float const yaw   = rot.y * 3.14159265f / 180.f;
    float const pitch = rot.x * 3.14159265f / 180.f;
    Vec3 const  dir{-std::sin(yaw) * std::cos(pitch), -std::sin(pitch), std::cos(yaw) * std::cos(pitch)};
    Vec3 const  end{eye.x + dir.x * 20.f, eye.y + dir.y * 20.f, eye.z + dir.z * 20.f};
    return player.getDimensionBlockSource().clip(
        eye,
        end,
        liquid,
        ShapeType::Outline,
        200,
        false,
        false,
        nullptr,
        [](BlockSource const&, Block const&, bool) { return true; },
        liquid
    );
}

}

EntityModule::EntityModule() : BaseModule("entity") {
    defaultNameColor  = legacyColor(ChatFormatting::RED);
    defaultValueColor = legacyColor(ChatFormatting::YELLOW);
    nameColor         = defaultNameColor;
    valueColor        = defaultValueColor;
    totalColor        = defaultTotalColor;
    addLine("particles");
    addLine("entities", "format.betterf3.total", true);
    for (char const* c : kMobCategories) addLine(c);
    mLines[0]->inReducedDebug = true;
    mLines[1]->inReducedDebug = true;
}

void EntityModule::update(ClientInstance& client) {
    auto* player = client.getLocalPlayer();
    if (!player) return;
    Level& level  = player->getLevel();
    auto   actors = level.getRuntimeActorList();
    mLines[1]->value(std::vector<Component>{
        utils::styledText(tr("text.betterf3.line.rendered"), valueColor),
        utils::styledText(tr("text.betterf3.line.total"), totalColor),
        utils::styledText(std::to_string(RenderStats::get().actorQueue), valueColor),
        utils::styledText(std::to_string(actors.size()), totalColor)});

    std::map<std::string, int> counts;
    for (auto* a : actors) {
        if (!a || a == player) continue;
        if ((static_cast<int>(a->getEntityTypeId()) & 0x100) == 0) continue;
        ++counts[entityCategoryName(*a)];
    }
    for (size_t i = 0; i < kMobCategories.size(); ++i) mLines[i + 2]->value(std::to_string(counts[kMobCategories[i]]));

    std::uint64_t particles = 0;
    if (auto* lr = client.getLevelRenderer(); lr) {
        if (auto* pse = lr->mParticleSystemEngine->get(); pse) particles = pse->mTotalParticleCount;
    }
    mLines[0]->value(std::to_string(particles));
}

void EntityModule::loadModuleConfig(Json const& c) { totalColor = readColor(c, "total_entities_color", defaultTotalColor); }
void EntityModule::saveModuleConfig(Json& c) { writeColor(c, "total_entities_color", totalColor); }

SoundModule::SoundModule() : BaseModule("sound") {
    defaultNameColor  = legacyColor(ChatFormatting::GOLD);
    defaultValueColor = legacyColor(ChatFormatting::AQUA);
    nameColor         = defaultNameColor;
    valueColor        = defaultValueColor;
    maximumColor      = defaultMaximumColor;
    addLine("sounds", "format.betterf3.total", true);
    addLine("ambient_sounds", "format.betterf3.total", true);
    addLine("mood").active = false;
}

void SoundModule::update(ClientInstance&) {
    auto const s = SoundStats::get();
    if (!s.valid) {
        mLines[0]->active = false;
        mLines[1]->active = false;
        return;
    }
    std::string const playing = tr("text.betterf3.line.playing");
    std::string const maximum = tr("text.betterf3.line.maximum");
    auto values = [&](int n) {
        return std::vector<Component>{
            utils::styledText(playing, valueColor),
            utils::styledText(maximum, maximumColor),
            utils::styledText(std::to_string(n), valueColor),
            utils::styledText(std::to_string(s.maxChannels), maximumColor)};
    };
    mLines[0]->value(values(s.playing));
    mLines[1]->value(values(s.streaming));
}

void SoundModule::loadModuleConfig(Json const& c) { maximumColor = readColor(c, "maximum_color", defaultMaximumColor); }
void SoundModule::saveModuleConfig(Json& c) { writeColor(c, "maximum_color", maximumColor); }

SystemModule::SystemModule() : BaseModule("system") {
    defaultNameColor  = legacyColor(ChatFormatting::GOLD);
    defaultValueColor = legacyColor(ChatFormatting::AQUA);
    nameColor         = defaultNameColor;
    valueColor        = defaultValueColor;
    memoryColorToggle = defaultMemoryColorToggle;
    timeFormat        = defaultTimeFormat;
    for (char const* id : {"time", "java_version", "memory_usage", "allocation_rate", "allocated_memory", "cpu", "display", "gpu", "gpu_utilization", "graphics_api", "gpu_driver"}) {
        addLine(id).inReducedDebug = true;
    }
}

long long SystemModule::allocationRate(long long usedBytes) {
    long long const now = utils::millis();
    if (now - mLastCalculated >= 500) {
        if (mLastCalculated != 0 && usedBytes >= mAllocatedBytes) {
            double const perSec = 1000.0 / static_cast<double>(now - mLastCalculated);
            mAllocationRate     = std::llround(static_cast<double>(usedBytes - mAllocatedBytes) * perSec);
        }
        mLastCalculated = now;
        mAllocatedBytes = usedBytes;
    }
    return mAllocationRate;
}

void SystemModule::update(ClientInstance&) {
    auto time = utils::formatNow(timeFormat);
    if (!time) {
        timeFormat = defaultTimeFormat;
        time       = utils::formatNow(timeFormat);
    }
    mLines[0]->value(time.value_or(""));
    mLines[1]->value(SystemInfo::osName() + " (x64)");

    long long used = 0, max = 0;
    if (auto app = ll::service::getAppPlatform()) {
        used = static_cast<long long>(app->getUsedMemory());
        long long const phys = static_cast<long long>(app->getTotalPhysicalMemory());
        max = static_cast<long long>(app->getMemoryLimit());
        if (max <= 0 || (phys > 0 && max > phys)) max = phys;
    }
    if (max <= 0) max = 1;
    long long const committed = static_cast<long long>(SystemInfo::processCommit());
    std::string const memoryUsage = spaceD(used * 100 / max, 2) + "% " + zeroD(used / 1024 / 1024, 3) + "/" + zeroD(max / 1024 / 1024, 3) + " MB";
    mLines[2]->value(memoryColorToggle ? std::string(kSection) + static_cast<char>(utils::percentColor(static_cast<int>(used * 100 / max))) + memoryUsage : memoryUsage);
    mLines[3]->value(spaceD(allocationRate(used) / 1024 / 1024, 2) + " MB/s");
    mLines[4]->value(spaceD(committed * 100 / max, 2) + "% " + zeroD(committed / 1024 / 1024, 3) + "MB");
    mLines[5]->value(SystemInfo::cpuInfo());

    auto const& gpu = SystemInfo::gpu();
    int         w = 0, h = 0;
    if (auto app = ll::service::getAppPlatform()) {
        w = app->getScreenWidth();
        h = app->getScreenHeight();
    }
    mLines[6]->value(std::to_string(w) + " x " + std::to_string(h) + " (" + gpu.vendor + ")");
    mLines[7]->value(gpu.name);
    if (auto u = SystemInfo::gpuUtilization()) {
        mLines[8]->value(*u > 100.0 ? std::string(kSection) + "c100%" : std::to_string(std::llround(*u)) + "%");
    } else {
        mLines[8]->value("N/A");
    }
    mLines[9]->value(SystemInfo::graphicsApi());
    mLines[10]->value(gpu.driver);
}

void SystemModule::loadModuleConfig(Json const& c) {
    memoryColorToggle = c.value("memory_color_toggle", defaultMemoryColorToggle);
    timeFormat        = c.value("time_format", defaultTimeFormat);
}

void SystemModule::saveModuleConfig(Json& c) {
    c["memory_color_toggle"] = memoryColorToggle;
    c["time_format"]         = timeFormat;
}

TargetModule::TargetModule() : BaseModule("target") {
    defaultNameColor  = 0x00AAFF;
    defaultValueColor = legacyColor(ChatFormatting::YELLOW);
    nameColor         = defaultNameColor;
    valueColor        = defaultValueColor;
    addLine("targeted_block");
    addLine("id_block");
    addLine<DebugLineList>("block_states");
    addLine<DebugLineList>("block_tags");
    addLine("nothing").active = false;
    addLine("targeted_fluid");
    addLine("id_fluid");
    addLine<DebugLineList>("fluid_states");
    addLine<DebugLineList>("fluid_tags").active = false;
    addLine("nothing2").active = false;
    addLine("targeted_entity");
}

void TargetModule::update(ClientInstance& client) {
    auto* player = client.getLocalPlayer();
    if (!player) return;
    BlockSource& bs = player->getDimensionBlockSource();

    HitResult const blockHit = pick(*player, false);
    if (blockHit.mType == HitResultType::Tile) {
        Block const& b = bs.getBlock(blockHit.mBlock);
        mLines[0]->value(posString(blockHit.mBlock));
        mLines[1]->value(b.getTypeName());
        static_cast<DebugLineList&>(*mLines[2]).values(blockStates(b));
        static_cast<DebugLineList&>(*mLines[3]).values(blockTags(b));
    } else {
        for (int i = 0; i < 5; ++i) mLines[i]->active = false;
    }

    HitResult const fluidHit = pick(*player, true);
    if (fluidHit.mIsHitLiquid) {
        Block const& liquid = bs.getLiquidBlock(fluidHit.mLiquid);
        mLines[5]->value(posString(fluidHit.mLiquid));
        mLines[6]->value(liquid.getTypeName());
        static_cast<DebugLineList&>(*mLines[7]).values(blockStates(liquid));
    } else {
        for (int i = 5; i < 10; ++i) mLines[i]->active = false;
    }

    HitResult const& hr = player->getLevel().getHitResult();
    Actor*           e  = hr.mType == HitResultType::Entity ? hr.getEntity() : nullptr;
    if (e) mLines[10]->value(e->getTypeName());
    else mLines[10]->active = false;
}

MiscLeftModule::MiscLeftModule() : BaseModule("miscleft") {
    defaultNameColor  = 0xFDFD96;
    defaultValueColor = legacyColor(ChatFormatting::AQUA);
    nameColor         = defaultNameColor;
    valueColor        = defaultValueColor;
    addLine<DebugLineList>("misc_left").inReducedDebug = true;
}

void MiscLeftModule::update(ClientInstance&) {
    auto lines = api::collectLines(false);
    std::erase_if(lines, [](std::string const& s) { return s.empty(); });
    static_cast<DebugLineList&>(*mLines[0]).values(std::move(lines));
}

MiscRightModule::MiscRightModule() : BaseModule("miscright") {
    defaultNameColor  = 0xFDFD96;
    defaultValueColor = legacyColor(ChatFormatting::AQUA);
    nameColor         = defaultNameColor;
    valueColor        = defaultValueColor;
    addLine<DebugLineList>("misc_right").inReducedDebug = true;
}

void MiscRightModule::update(ClientInstance&) {
    std::vector<std::string> lines{"BetterF3 Version: " + utils::modVersion()};
    for (auto& s : api::collectLines(true)) {
        if (!s.empty()) lines.push_back(std::move(s));
    }
    static_cast<DebugLineList&>(*mLines[0]).values(std::move(lines));
}

}
