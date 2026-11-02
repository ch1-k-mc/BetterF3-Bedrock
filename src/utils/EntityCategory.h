// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <string>

class Actor;

namespace better_f3 {

inline constexpr std::array<char const*, 8> kMobCategories{
    "monster", "creature", "ambient", "axolotls", "underground_water_creature", "water_creature", "water_ambient", "misc"};

[[nodiscard]] std::string entityCategoryName(Actor const& actor);

}
