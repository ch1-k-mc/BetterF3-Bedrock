// SPDX-License-Identifier: MIT
#include "utils/Component.h"

#include <cctype>

#include "utils/Lang.h"

namespace better_f3 {

Component Component::literal(std::string const& s) {
    Component                c;
    std::optional<TextColor> color;
    std::string              cur;
    auto flush = [&] {
        if (!cur.empty()) c.mSegments.push_back({cur, color});
        cur.clear();
    };
    for (size_t i = 0; i < s.size(); ++i) {
        if (s.compare(i, kSection.size(), kSection) == 0 && i + kSection.size() < s.size()) {
            char const code = s[i + kSection.size()];
            flush();
            if (auto rgb = legacyRgb(code)) color = rgb;
            else if (code == 'r' || code == 'R') color.reset();
            i += kSection.size();
            continue;
        }
        cur.push_back(s[i]);
    }
    flush();
    return c;
}

Component Component::styled(std::string const& s, TextColor color) { return literal(s).withColor(color); }

Component Component::translatable(std::string const& key, std::vector<Component> const& args) {
    std::string const fmt = Lang::get().get(key);
    Component         out;
    std::string       text;
    size_t            next = 0;
    auto              raw  = [&] { return literal(fmt); };
    auto              flushText = [&] {
        if (!text.empty()) out.append(literal(text));
        text.clear();
    };
    for (size_t i = 0; i < fmt.size(); ++i) {
        if (fmt[i] != '%') {
            text.push_back(fmt[i]);
            continue;
        }
        size_t j     = i + 1;
        int    index = -1;
        size_t d     = j;
        while (d < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[d]))) ++d;
        if (d > j && d < fmt.size() && fmt[d] == '$') {
            index = std::stoi(fmt.substr(j, d - j)) - 1;
            j     = d + 1;
        }
        if (j >= fmt.size()) return raw();
        if (fmt[j] == '%' && index < 0) {
            text.push_back('%');
            i = j;
            continue;
        }
        while (j < fmt.size() && (fmt[j] == '.' || std::isdigit(static_cast<unsigned char>(fmt[j])))) ++j;
        if (j >= fmt.size()) return raw();
        char const conv = fmt[j];
        if (conv != 's' && conv != 'd' && conv != 'f') return raw();
        flushText();
        size_t const argIndex = index >= 0 ? static_cast<size_t>(index) : next++;
        if (argIndex >= args.size()) return raw();
        out.append(args[argIndex]);
        i = j;
    }
    flushText();
    return out;
}

Component& Component::append(Component const& other) {
    mSegments.insert(mSegments.end(), other.mSegments.begin(), other.mSegments.end());
    return *this;
}

Component& Component::withColor(TextColor color) {
    for (auto& s : mSegments) {
        if (!s.color) s.color = color;
    }
    return *this;
}

std::string Component::getString() const {
    std::string s;
    for (auto const& seg : mSegments) s += seg.text;
    return s;
}

bool Component::empty() const {
    for (auto const& seg : mSegments) {
        if (!seg.text.empty()) return false;
    }
    return true;
}

}
