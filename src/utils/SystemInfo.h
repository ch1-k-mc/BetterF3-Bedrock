// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace better_f3::SystemInfo {

struct Gpu {
    std::string name;
    std::string vendor;
    std::string driver;
};

[[nodiscard]] std::string const& cpuInfo();
[[nodiscard]] Gpu const&         gpu();
[[nodiscard]] std::string        graphicsApi();
[[nodiscard]] std::optional<double> gpuUtilization();
[[nodiscard]] std::uint64_t processCommit();
[[nodiscard]] std::string   osName();

bool postFocusMessage(bool lost);

}
