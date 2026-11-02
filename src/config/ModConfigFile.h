// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/config/ModConfigFile.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#pragma once

#include <filesystem>

namespace better_f3::ModConfigFile {

void setPath(std::filesystem::path path);
[[nodiscard]] std::filesystem::path const& path();

void load();
void save();

}
