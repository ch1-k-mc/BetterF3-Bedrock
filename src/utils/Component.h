// SPDX-License-Identifier: MIT
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "utils/TextColor.h"

namespace better_f3 {

struct Segment {
    std::string              text;
    std::optional<TextColor> color;
};

class Component {
public:
    Component() = default;

    static Component literal(std::string const& s);
    static Component styled(std::string const& s, TextColor color);
    static Component translatable(std::string const& key, std::vector<Component> const& args = {});

    Component& append(Component const& other);
    Component& withColor(TextColor color);

    [[nodiscard]] std::string                 getString() const;
    [[nodiscard]] std::vector<Segment> const& segments() const { return mSegments; }
    [[nodiscard]] bool                        empty() const;

private:
    std::vector<Segment> mSegments;
};

}
