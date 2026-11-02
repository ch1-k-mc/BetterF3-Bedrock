// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <vector>

namespace better_f3 {
class Gui2d;
}

namespace better_f3::DebugCharts {

void onFrame(Gui2d& g);

bool onDigit(int digit);

void pushPing(float ms);
void pushTick(float ms);
void pushBandwidth(float bytesPerSecond);

[[nodiscard]] std::string profilerPath();
[[nodiscard]] std::vector<std::string> profilerEntries();
[[nodiscard]] std::size_t frameSamples();
[[nodiscard]] std::size_t tickSamples();

}
