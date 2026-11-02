// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace better_f3 {

using TextColor = std::uint32_t;

enum class ChatFormatting : char {
    BLACK        = '0',
    DARK_BLUE    = '1',
    DARK_GREEN   = '2',
    DARK_AQUA    = '3',
    DARK_RED     = '4',
    DARK_PURPLE  = '5',
    GOLD         = '6',
    GRAY         = '7',
    DARK_GRAY    = '8',
    BLUE         = '9',
    GREEN        = 'a',
    AQUA         = 'b',
    RED          = 'c',
    LIGHT_PURPLE = 'd',
    YELLOW       = 'e',
    WHITE        = 'f',
};

[[nodiscard]] constexpr std::optional<TextColor> legacyRgb(char code) {
    switch (code) {
    case '0': return 0x000000;
    case '1': return 0x0000AA;
    case '2': return 0x00AA00;
    case '3': return 0x00AAAA;
    case '4': return 0xAA0000;
    case '5': return 0xAA00AA;
    case '6': return 0xFFAA00;
    case '7': return 0xAAAAAA;
    case '8': return 0x555555;
    case '9': return 0x5555FF;
    case 'a': case 'A': return 0x55FF55;
    case 'b': case 'B': return 0x55FFFF;
    case 'c': case 'C': return 0xFF5555;
    case 'd': case 'D': return 0xFF55FF;
    case 'e': case 'E': return 0xFFFF55;
    case 'f': case 'F': return 0xFFFFFF;
    default: return std::nullopt;
    }
}

[[nodiscard]] constexpr TextColor legacyColor(ChatFormatting f) { return legacyRgb(static_cast<char>(f)).value_or(0xFFFFFF); }

inline constexpr std::string_view kSection = "\xC2\xA7";

}
