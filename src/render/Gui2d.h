// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>

#include "utils/Component.h"

class ClientInstance;
class Font;
class MinecraftUIRenderContext;

namespace better_f3 {

class Gui2d {
public:
    Gui2d(MinecraftUIRenderContext& ctx, ClientInstance& ci);
    ~Gui2d();
    Gui2d(Gui2d const&)            = delete;
    Gui2d& operator=(Gui2d const&) = delete;

    [[nodiscard]] float width() const { return mWidth; }
    [[nodiscard]] float height() const { return mHeight; }

    void fill(float x1, float y1, float x2, float y2, std::uint32_t argb);
    void text(Component const& c, float x, float y, std::uint32_t defaultRgb, bool shadow, float scale = 1.0f);
    [[nodiscard]] float width(std::string const& s);
    [[nodiscard]] float width(Component const& c) { return width(c.getString()); }
    void                flush();

private:
    Font& font();

    MinecraftUIRenderContext& mCtx;
    ClientInstance&           mClient;
    Font*                     mFont        = nullptr;
    bool                      mTextPending = false;
    float                     mWidth       = 0;
    float                     mHeight      = 0;
};

}
