// SPDX-License-Identifier: MIT
// Based on TreyRuffy/BetterF3 common/src/main/java/me/cominixo/betterf3/utils/DebugLine.java (architectury/26.3 @d7c9ba6), ported to C++ and modified.
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "utils/Component.h"

namespace better_f3 {

class DebugLine {
public:
    using Value = std::variant<std::string, Component, std::vector<Component>>;

    explicit DebugLine(std::string id);
    DebugLine(std::string id, std::string formatString, bool isCustom);
    virtual ~DebugLine() = default;

    Component toText(TextColor nameColor, TextColor valueColor);
    Component toTextCustom(TextColor nameColor);

    void value(Value v);
    void value(char const* s) { value(Value{std::string(s)}); }
    void format(std::string f) { mFormat = std::move(f); }
    [[nodiscard]] std::string name();
    void name(std::string n) { mName = std::move(n); }
    [[nodiscard]] std::string const& id() const { return mId; }
    [[nodiscard]] virtual std::size_t cacheStateHash() const;

    bool active         = true;
    bool enabled        = true;
    bool isCustom       = false;
    bool inReducedDebug = false;

protected:
    Value                      mValue = std::string();
    std::string                mFormat;
    std::string                mId;
    std::optional<std::string> mName;
};

class DebugLineList : public DebugLine {
public:
    explicit DebugLineList(std::string id) : DebugLine(std::move(id)) {}

    void values(std::vector<std::string> v) {
        mValues = std::move(v);
        active  = true;
    }
    [[nodiscard]] std::vector<std::string> const& values() const { return mValues; }
    [[nodiscard]] std::vector<Component>          toTexts(TextColor nameColor, TextColor valueColor) const;
    [[nodiscard]] std::size_t cacheStateHash() const override;

private:
    std::vector<std::string> mValues;
};

}
