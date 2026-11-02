// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/utils/DebugLine.java, DebugLineList.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#include "utils/DebugLine.h"

#include <functional>

#include "utils/Lang.h"
#include "utils/Utils.h"

namespace better_f3 {

namespace {

void hashCombine(std::size_t& h, std::size_t v) { h = h * 31 + v; }

std::size_t hashComponent(Component const& c) {
    std::size_t h = 0;
    for (auto const& s : c.segments()) {
        hashCombine(h, std::hash<std::string>{}(s.text));
        hashCombine(h, s.color ? *s.color + 1 : 0);
    }
    return h;
}

}

DebugLine::DebugLine(std::string id) : mFormat("format.betterf3.default_format"), mId(std::move(id)) {}

DebugLine::DebugLine(std::string id, std::string formatString, bool custom)
: mFormat(std::move(formatString)),
  mId(std::move(id)) {
    isCustom = custom;
}

Component DebugLine::toText(TextColor nameColor, TextColor valueColor) {
    Component const nameStyled = utils::styledText(name(), nameColor);
    Component       valueStyled;
    bool            empty = false;
    if (auto const* c = std::get_if<Component>(&mValue)) {
        valueStyled = *c;
    } else if (auto const* s = std::get_if<std::string>(&mValue)) {
        valueStyled = utils::styledText(*s, valueColor);
        empty       = s->empty();
    } else {
        Component joined;
        for (auto const& v : std::get<std::vector<Component>>(mValue)) joined.append(v);
        valueStyled = joined.withColor(valueColor);
    }
    if (empty) active = false;
    return Component::translatable(mFormat, {nameStyled, valueStyled});
}

Component DebugLine::toTextCustom(TextColor nameColor) {
    std::string const currentName = name();
    if (auto const* list = std::get_if<std::vector<Component>>(&mValue)) {
        std::vector<Component> values;
        if (!currentName.empty()) values.push_back(utils::styledText(currentName, nameColor));
        values.insert(values.end(), list->begin(), list->end());
        return Component::translatable(mFormat, values).withColor(nameColor);
    }
    Component v;
    if (auto const* c = std::get_if<Component>(&mValue)) v = *c;
    else v = Component::literal(std::get<std::string>(mValue));
    return Component::translatable(mFormat, {Component::literal(currentName), v});
}

void DebugLine::value(Value v) {
    active = true;
    mValue = std::move(v);
}

std::string DebugLine::name() {
    if (mName) return *mName;
    if (mId.empty()) {
        mFormat = "%s%s";
        return "";
    }
    return Lang::get().get("text.betterf3.line." + mId);
}

std::size_t DebugLine::cacheStateHash() const {
    std::size_t h = std::hash<std::string>{}(mId);
    hashCombine(h, std::hash<std::string>{}(mFormat));
    hashCombine(h, mName ? std::hash<std::string>{}(*mName) : 0);
    hashCombine(h, active);
    hashCombine(h, enabled);
    hashCombine(h, isCustom);
    hashCombine(h, inReducedDebug);
    if (auto const* s = std::get_if<std::string>(&mValue)) hashCombine(h, std::hash<std::string>{}(*s));
    else if (auto const* c = std::get_if<Component>(&mValue)) hashCombine(h, hashComponent(*c));
    else {
        auto const& list = std::get<std::vector<Component>>(mValue);
        hashCombine(h, list.size());
        for (auto const& c : list) hashCombine(h, hashComponent(c));
    }
    return h;
}

std::size_t DebugLineList::cacheStateHash() const {
    std::size_t h = DebugLine::cacheStateHash();
    hashCombine(h, mValues.size());
    for (auto const& v : mValues) hashCombine(h, std::hash<std::string>{}(v));
    return h;
}

std::vector<Component> DebugLineList::toTexts(TextColor nameColor, TextColor valueColor) const {
    std::vector<Component> texts;
    texts.reserve(mValues.size());
    for (auto const& v : mValues) texts.push_back(utils::formattedFromString(v, nameColor, valueColor));
    return texts;
}

}
