// SPDX-License-Identifier: MIT
// C API for other LeviLamina mods to add lines to the Misc Left / Misc Right modules.
// If BetterF3 is not installed, GetProcAddress returns null.
//   using SetLines = void (*)(char const* owner, int right, char const* const* lines, int count);
//   auto f = reinterpret_cast<SetLines>(GetProcAddress(GetModuleHandleW(L"BetterF3.dll"), "better_f3_set_lines"));
//   char const* lines[] = {"My Mod: on"};  // "Name: Value" is colored as name / value
//   if (f) f("MyMod", 0, lines, 1);       // 0 = left, 1 = right
// Each call replaces all lines of the owner. Thread-safe.
#pragma once

#include <string>
#include <vector>

#define BETTER_F3_API extern "C" __declspec(dllexport)

BETTER_F3_API int  better_f3_api_version();
BETTER_F3_API void better_f3_set_lines(char const* owner, int right, char const* const* lines, int count);
BETTER_F3_API void better_f3_clear_lines(char const* owner);
BETTER_F3_API int  better_f3_is_visible();

namespace better_f3::api {

[[nodiscard]] std::vector<std::string> collectLines(bool right);

}
