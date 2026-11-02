// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/utils/Utils.java, FpsEnum.java, PositionEnum.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "utils/Component.h"
#include "utils/TextColor.h"

namespace better_f3 {

enum class FpsEnum { HIGH, MEDIUM, LOW };
enum class PositionEnum { RIGHT, LEFT, BOTH };
enum class Direction { NORTH, SOUTH, WEST, EAST, UP, DOWN };

namespace utils {

inline constexpr int START_X_POS        = 200;
inline int           xPos               = START_X_POS;
inline std::int64_t  lastAnimationUpdate = 0;
inline bool          closingAnimation   = false;

[[nodiscard]] FpsEnum        fpsColor(int currentFps);
[[nodiscard]] ChatFormatting percentColor(int percent);
[[nodiscard]] std::string    facingString(Direction facing);
[[nodiscard]] Component      styledText(std::string const& s, TextColor color);
[[nodiscard]] Component formattedFromString(std::string const& s, TextColor nameColor, TextColor valueColor);
[[nodiscard]] std::string enumToString(std::string s);
[[nodiscard]] std::string propertyToString(std::string const& name, std::string const& value);

[[nodiscard]] std::string javaFormat(std::string const& fmt, std::vector<std::string> const& args);
[[nodiscard]] std::string i18n(std::string const& key, std::vector<std::string> const& args = {});
[[nodiscard]] std::string fixed(double v, int digits);
[[nodiscard]] float wrapDegrees(float deg);

[[nodiscard]] std::optional<std::string> formatNow(std::string const& pattern);

[[nodiscard]] std::string const& modVersion();
void                             modVersion(std::string v);

[[nodiscard]] std::int64_t millis();

}
}
