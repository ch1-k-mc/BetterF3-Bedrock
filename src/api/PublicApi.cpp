// SPDX-License-Identifier: MIT
#include "api/PublicApi.h"

#include <map>
#include <mutex>

#include "utils/DebugOverlay.h"

namespace {

struct OwnerLines {
    std::vector<std::string> left;
    std::vector<std::string> right;
};

std::mutex                        gMu;
std::map<std::string, OwnerLines> gLines;

}

BETTER_F3_API int better_f3_api_version() { return 1; }

BETTER_F3_API void better_f3_set_lines(char const* owner, int right, char const* const* lines, int count) {
    if (!owner) return;
    std::vector<std::string> v;
    for (int i = 0; lines && i < count; ++i) v.emplace_back(lines[i] ? lines[i] : "");
    std::lock_guard lk(gMu);
    auto&           o = gLines[owner];
    (right ? o.right : o.left) = std::move(v);
}

BETTER_F3_API void better_f3_clear_lines(char const* owner) {
    if (!owner) return;
    std::lock_guard lk(gMu);
    gLines.erase(owner);
}

BETTER_F3_API int better_f3_is_visible() { return better_f3::DebugOverlay::isOverlayVisible ? 1 : 0; }

namespace better_f3::api {

std::vector<std::string> collectLines(bool right) {
    std::vector<std::string> out;
    std::lock_guard          lk(gMu);
    for (auto const& [owner, o] : gLines) {
        auto const& v = right ? o.right : o.left;
        out.insert(out.end(), v.begin(), v.end());
    }
    return out;
}

}
