// SPDX-License-Identifier: MIT
#include "utils/EntityCategory.h"

#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorType.h"

namespace better_f3 {

namespace {
bool has(int type, ActorType family) {
    int const f = static_cast<int>(family);
    return (type & f) == f;
}
}

std::string entityCategoryName(Actor const& actor) {
    auto const t    = actor.getEntityTypeId();
    int const  type = static_cast<int>(t);
    switch (t) {
    case ActorType::Axolotl: return "axolotls";
    case ActorType::GlowSquid: return "underground_water_creature";
    case ActorType::Fish:
    case ActorType::Salmon:
    case ActorType::Pufferfish:
    case ActorType::Tropicalfish:
    case ActorType::Tadpole: return "water_ambient";
    case ActorType::Allay: return "creature";
    default: break;
    }
    if (has(type, ActorType::WaterAnimal)) return "water_creature";
    if (has(type, ActorType::Monster)) return "monster";
    if (has(type, ActorType::Ambient)) return "ambient";
    if (has(type, ActorType::Animal)) return "creature";
    return "misc";
}

}
