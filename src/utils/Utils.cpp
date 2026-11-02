// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/utils/Utils.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#include "utils/Utils.h"

#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>

#include "utils/Lang.h"

namespace better_f3::utils {

namespace {
std::string gModVersion;
}

FpsEnum fpsColor(int currentFps) {
    if (currentFps >= 60) return FpsEnum::HIGH;
    if (currentFps >= 20) return FpsEnum::MEDIUM;
    return FpsEnum::LOW;
}

ChatFormatting percentColor(int percent) {
    if (percent >= 90) return ChatFormatting::RED;
    if (percent >= 60) return ChatFormatting::YELLOW;
    return ChatFormatting::GREEN;
}

std::string facingString(Direction facing) {
    switch (facing) {
    case Direction::NORTH: return Lang::get().get("text.betterf3.line.negative_z");
    case Direction::SOUTH: return Lang::get().get("text.betterf3.line.positive_z");
    case Direction::WEST: return Lang::get().get("text.betterf3.line.negative_x");
    case Direction::EAST: return Lang::get().get("text.betterf3.line.positive_x");
    default: return "";
    }
}

Component styledText(std::string const& s, TextColor color) { return Component::styled(s, color); }

Component formattedFromString(std::string const& s, TextColor nameColor, TextColor valueColor) {
    auto const colon = s.find(':');
    if (colon == std::string::npos) return Component::literal(s);
    std::string rest = s.substr(colon + 1);
    while (!rest.empty() && rest.back() == ':') rest.pop_back();
    Component out = styledText(s.substr(0, colon), nameColor);
    out.append(Component::literal(":"));
    out.append(styledText(rest, valueColor));
    return out;
}

std::string enumToString(std::string s) {
    for (auto& c : s) {
        if (c == '_') c = ' ';
    }
    if (!s.empty()) s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    return s;
}

std::string propertyToString(std::string const& name, std::string const& value) {
    std::string v = value;
    if (value == "true") v = std::string(kSection) + "a" + value;
    else if (value == "false") v = std::string(kSection) + "c" + value;
    return name + ": " + v;
}

std::string javaFormat(std::string const& fmt, std::vector<std::string> const& args) {
    std::string out;
    size_t      next = 0;
    for (size_t i = 0; i < fmt.size(); ++i) {
        if (fmt[i] != '%' || i + 1 >= fmt.size()) {
            out.push_back(fmt[i]);
            continue;
        }
        size_t j = i + 1;
        if (fmt[j] == '%') {
            out.push_back('%');
            i = j;
            continue;
        }
        int    index = -1;
        size_t d     = j;
        while (d < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[d]))) ++d;
        if (d > j && d < fmt.size() && fmt[d] == '$') {
            index = std::stoi(fmt.substr(j, d - j)) - 1;
            j     = d + 1;
        }
        while (j < fmt.size() && (fmt[j] == '.' || std::isdigit(static_cast<unsigned char>(fmt[j])))) ++j;
        if (j >= fmt.size()) break;
        size_t const argIndex = index >= 0 ? static_cast<size_t>(index) : next++;
        if (argIndex < args.size()) out += args[argIndex];
        i = j;
    }
    return out;
}

std::string i18n(std::string const& key, std::vector<std::string> const& args) { return javaFormat(Lang::get().get(key), args); }

std::string fixed(double v, int digits) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", digits, v);
    return buf;
}

float wrapDegrees(float deg) {
    float f = std::fmod(deg, 360.0f);
    if (f >= 180.0f) f -= 360.0f;
    if (f < -180.0f) f += 360.0f;
    return f;
}

std::optional<std::string> formatNow(std::string const& pattern) {
    auto const        now = std::chrono::system_clock::now();
    std::time_t const t   = std::chrono::system_clock::to_time_t(now);
    int const         ms  = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000);
    std::tm           tm{};
    localtime_s(&tm, &t);
    static char const* const kDays[]   = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static char const* const kMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    auto pad = [](int v, size_t n) {
        std::string s = std::to_string(v);
        while (s.size() < n) s.insert(s.begin(), '0');
        return s;
    };
    constexpr char kQuote = 0x27;
    std::string    out;
    for (size_t i = 0; i < pattern.size();) {
        char const c = pattern[i];
        if (c == kQuote) {
            size_t j = i + 1;
            if (j < pattern.size() && pattern[j] == kQuote) {
                out.push_back(kQuote);
                i += 2;
                continue;
            }
            while (j < pattern.size() && pattern[j] != kQuote) out.push_back(pattern[j++]);
            if (j >= pattern.size()) return std::nullopt;
            i = j + 1;
            continue;
        }
        if (!std::isalpha(static_cast<unsigned char>(c))) {
            out.push_back(c);
            ++i;
            continue;
        }
        size_t n = 0;
        while (i + n < pattern.size() && pattern[i + n] == c) ++n;
        switch (c) {
        case 'y':
        case 'u': out += n == 2 ? pad((tm.tm_year + 1900) % 100, 2) : pad(tm.tm_year + 1900, n); break;
        case 'M': out += n >= 3 ? std::string(kMonths[tm.tm_mon]) : pad(tm.tm_mon + 1, n); break;
        case 'd': out += pad(tm.tm_mday, n); break;
        case 'H': out += pad(tm.tm_hour, n); break;
        case 'h': out += pad(tm.tm_hour % 12 == 0 ? 12 : tm.tm_hour % 12, n); break;
        case 'm': out += pad(tm.tm_min, n); break;
        case 's': out += pad(tm.tm_sec, n); break;
        case 'S': out += pad(ms, 3).substr(0, std::min<size_t>(n, 3)); break;
        case 'a': out += tm.tm_hour < 12 ? "AM" : "PM"; break;
        case 'E': out += kDays[tm.tm_wday]; break;
        default: return std::nullopt;
        }
        i += n;
    }
    return out;
}

std::string const& modVersion() { return gModVersion; }
void               modVersion(std::string v) { gModVersion = std::move(v); }

std::int64_t millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

}
