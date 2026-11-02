// SPDX-License-Identifier: MIT
#include "render/Gui2d.h"

#include <algorithm>
#include <optional>
#include <string_view>

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/CaretMeasureData.h"
#include "mc/client/gui/FontHandle.h"
#include "mc/client/gui/GuiData.h"
#include "mc/client/gui/ScreenSizeData.h"
#include "mc/client/gui/TextAlignment.h"
#include "mc/client/gui/TextMeasureData.h"
#include "mc/client/renderer/screen/MinecraftUIRenderContext.h"
#include "mc/deps/core/math/Color.h"
#include "mc/deps/core/math/Vec2.h"
#include "mc/deps/input/RectangleArea.h"

namespace better_f3 {

namespace {

::mce::Color rgba(std::uint32_t argb) {
    return ::mce::Color(
        static_cast<float>((argb >> 16) & 0xFF) / 255.f,
        static_cast<float>((argb >> 8) & 0xFF) / 255.f,
        static_cast<float>(argb & 0xFF) / 255.f,
        static_cast<float>((argb >> 24) & 0xFF) / 255.f
    );
}

template <class F>
void forEachRun(std::string const& s, F&& f) {
    size_t start = 0;
    while (start < s.size()) {
        bool const ascii = static_cast<unsigned char>(s[start]) < 0x80;
        size_t     end   = start;
        while (end < s.size() && (static_cast<unsigned char>(s[end]) < 0x80) == ascii) ++end;
        f(std::string_view(s).substr(start, end - start));
        start = end;
    }
}

}

Gui2d::Gui2d(MinecraftUIRenderContext& ctx, ClientInstance& ci) : mCtx(ctx), mClient(ci) {
    auto   gd = ci.getGuiData();
    ::Vec2 ui = gd->mScreenSizeData.get().clientUIScreenSize.get();
    mWidth    = ui.x;
    mHeight   = ui.y;
}

Gui2d::~Gui2d() { flush(); }

Font& Gui2d::font() {
    if (!mFont) {
        ::FontHandle handle = mClient.getFontHandle();
        mFont               = &handle.getFont();
    }
    return *mFont;
}

void Gui2d::flush() {
    if (!mTextPending) return;
    mTextPending = false;
    mCtx.flushText(0.0f, std::nullopt);
}

void Gui2d::fill(float x1, float y1, float x2, float y2, std::uint32_t argb) {
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    float const a = static_cast<float>((argb >> 24) & 0xFF) / 255.f;
    if (x1 == x2 || y1 == y2 || a <= 0.f) return;
    flush();
    ::RectangleArea r{x1, x2, y1, y2};
    mCtx.fillRectangle(r, rgba(argb | 0xFF000000), a);
}

void Gui2d::text(Component const& c, float x, float y, std::uint32_t defaultRgb, bool shadow, float scale) {
    float cx = x;
    for (auto const& seg : c.segments()) {
        if (seg.text.empty()) continue;
        std::uint32_t const rgb = seg.color.value_or(defaultRgb & 0xFFFFFF);
        forEachRun(seg.text, [&](std::string_view run) {
            ::RectangleArea    r{cx, mWidth * 4.f + cx, y, y + 10.f * scale};
            ::TextMeasureData  tmd{scale, 0.0f, shadow, false, false, ::ui::TextAlignment::Left};
            ::CaretMeasureData cmd{0, false};
            std::string        str(run);
            mCtx.drawText(font(), r, std::string(str), rgba(0xFF000000 | rgb), 1.0f, ::ui::TextAlignment::Left, tmd, cmd);
            mTextPending = true;
            cx += static_cast<float>(mCtx.getLineLength(font(), str, 1.0f, false)) * scale;
        });
    }
}

float Gui2d::width(std::string const& s) {
    float w = 0.f;
    forEachRun(s, [&](std::string_view run) { w += static_cast<float>(mCtx.getLineLength(font(), std::string(run), 1.0f, false)); });
    return w;
}

}
