// SPDX-License-Identifier: MIT
#include "graphs/DebugCharts.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <deque>
#include <functional>
#include <map>

#include "mc/profile/ProfilerLite.h"

#include "config/GeneralOptions.h"
#include "render/Gui2d.h"
#include "utils/DebugOverlay.h"
#include "utils/Utils.h"

namespace better_f3::DebugCharts {

namespace {

constexpr std::size_t kMaxSamples = 240;

struct Samples {
    std::deque<float> v;
    void              push(float x) {
        v.push_back(x);
        while (v.size() > kMaxSamples) v.pop_front();
    }
};

Samples gFrame;
Samples gTick;
Samples gPing;
Samples gBandwidth;

std::chrono::steady_clock::time_point gLastFrame{};
std::int64_t                          gLastTickNs = -1;

std::vector<ProfilerLite::ScopedData const*> gPath;
struct Entry {
    std::string name;
    double      percent       = 0;
    double      globalPercent = 0;
    std::uint32_t color       = 0;
    ProfilerLite::ScopedData const* node = nullptr;
};
std::vector<Entry>                              gEntries;
std::map<ProfilerLite::ScopedData const*, double> gLastInclusive;
std::int64_t                                    gLastPieUpdate = 0;
double                                          gRootDeltaShare = 1.0;

std::uint32_t colorOf(std::string const& name) {
    std::uint32_t h = 0;
    for (unsigned char c : name) h = h * 31 + c;
    return 0xFF000000 | ((h & 0x7F7F7F) << 1);
}

ProfilerLite::ScopedData const* findRoot() {
    auto& pl = ProfilerLite::gProfilerLiteInstance();
    for (auto* d : *pl.mCustomScopeDatas) {
        if (d && d->mName && std::string(d->mName) == "Minecraft Game - update") return d;
    }
    for (auto* d : *pl.mCustomScopeDatas) {
        if (d && d->mName && std::string(d->mName) == "---MainThread---") return d;
    }
    return nullptr;
}

void updatePie() {
    std::int64_t const now = utils::millis();
    if (now - gLastPieUpdate < 1000) return;
    gLastPieUpdate = now;
    if (gPath.empty()) {
        if (auto* r = findRoot()) gPath.push_back(r);
        else return;
    }
    auto delta = [](ProfilerLite::ScopedData const* d) {
        double const cur  = d->mTotalInclusiveTime;
        auto         it   = gLastInclusive.find(d);
        double const prev = it == gLastInclusive.end() ? cur : it->second;
        gLastInclusive[d] = cur;
        return std::max(0.0, cur - prev);
    };
    double const rootDelta = delta(gPath.front());
    double const nodeDelta = gPath.size() > 1 ? delta(gPath.back()) : rootDelta;
    std::vector<Entry> entries;
    double             used = 0;
    for (auto* c : *gPath.back()->mChildren) {
        if (!c) continue;
        double const d = delta(c);
        Entry        e;
        e.name          = c->mName ? c->mName : "?";
        e.percent       = nodeDelta > 0 ? d / nodeDelta * 100.0 : 0;
        e.globalPercent = rootDelta > 0 ? d / rootDelta * 100.0 : 0;
        e.color         = colorOf(e.name);
        e.node          = c;
        used += d;
        entries.push_back(e);
    }
    if (nodeDelta > used) {
        Entry e;
        e.name          = "unspecified";
        e.percent       = (nodeDelta - used) / nodeDelta * 100.0;
        e.globalPercent = rootDelta > 0 ? (nodeDelta - used) / rootDelta * 100.0 : 0;
        e.color         = colorOf(e.name);
        entries.push_back(e);
    }
    std::sort(entries.begin(), entries.end(), [](Entry const& a, Entry const& b) { return a.percent > b.percent; });
    gEntries        = std::move(entries);
    gRootDeltaShare = rootDelta > 0 ? nodeDelta / rootDelta : 0;
}

std::string pct(double v) {
    char b[32];
    std::snprintf(b, sizeof b, "%.2f%%", v);
    return b;
}

void drawPie(Gui2d& g) {
    updatePie();
    if (gPath.empty()) return;
    float const r     = 40.f;
    float const panel = 180.f;
    float const left  = g.width() - panel;
    float const cx    = left + panel / 2.f;
    float const lines = static_cast<float>(gEntries.size() + 2);
    float const cy    = g.height() - lines * 9.f - r * 0.5f - 6.f;
    g.fill(left, cy - r * 0.5f - 16.f, g.width(), g.height(), 0x90303030);
    std::vector<std::pair<double, std::uint32_t>> slices;
    double acc = 0;
    for (auto const& e : gEntries) {
        acc += e.percent / 100.0;
        slices.emplace_back(acc, e.color);
    }
    for (float y = -r; y < r; y += 2.f) {
        for (float x = -r; x < r; x += 2.f) {
            if (x * x + y * y > r * r) continue;
            double a = (std::atan2(x, -y) / (2.0 * 3.14159265358979)) ;
            if (a < 0) a += 1.0;
            std::uint32_t col = 0xFF404040;
            for (auto const& [end, c] : slices) {
                if (a <= end) {
                    col = c;
                    break;
                }
            }
            g.fill(cx + x, cy + y * 0.5f, cx + x + 2.f, cy + y * 0.5f + 1.f, col);
        }
    }
    std::string path = "root";
    for (size_t i = 1; i < gPath.size(); ++i) path += std::string(".") + (gPath[i]->mName ? gPath[i]->mName : "?");
    float y = cy - r * 0.5f - 14.f;
    g.text(Component::literal(path), left + 2.f, y, 0xFFFFFFFF, true);
    Component const total = Component::literal(pct(gRootDeltaShare * 100.0));
    g.text(total, g.width() - g.width(total) - 4.f, y, 0xFFFFFFFF, true);
    y = cy + r * 0.5f + 6.f;
    if (gPath.size() > 1) {
        g.text(Component::literal("[0] back"), left + 2.f, y, 0xFFE0E0E0, true);
        y += 9.f;
    }
    int number = 0;
    for (size_t i = 0; i < gEntries.size(); ++i) {
        auto const& e      = gEntries[i];
        char        idx[8] = "[-] ";
        if (e.node && ++number <= 9) std::snprintf(idx, sizeof idx, "[%d] ", number);
        std::string label = std::string(idx) + e.name;
        float const maxW  = panel - 96.f;
        while (label.size() > 4 && g.width(label) > maxW) label.pop_back();
        g.text(Component::literal(label), left + 2.f, y, e.color, true);
        Component const p1 = Component::literal(pct(e.percent));
        Component const p2 = Component::literal(pct(e.globalPercent));
        g.text(p1, g.width() - 48.f - g.width(p1), y, e.color, true);
        g.text(p2, g.width() - 4.f - g.width(p2), y, e.color, true);
        y += 9.f;
    }
}

std::uint32_t lerpColor(std::uint32_t a, std::uint32_t b, double t) {
    t        = std::clamp(t, 0.0, 1.0);
    auto ch  = [&](int s) { return static_cast<std::uint32_t>(std::lround(((a >> s) & 0xFF) * (1 - t) + ((b >> s) & 0xFF) * t)) << s; };
    return 0xFF000000 | ch(16) | ch(8) | ch(0);
}

std::uint32_t sampleColor(double v, double mid, double high) {
    if (v < mid) return lerpColor(0xFF00FF00, 0xFFFFFF00, v / mid);
    return lerpColor(0xFFFFFF00, 0xFFFF0000, (v - mid) / (high - mid));
}

std::string ms(double v) {
    char b[32];
    std::snprintf(b, sizeof b, "%.1f ms", v);
    return b;
}

struct ChartStyle {
    double                                       fullHeightValue;
    double                                       colorMid, colorHigh;
    std::vector<std::pair<double, std::string>>  lines;
    std::function<std::string(double)>           fmt;
};

void drawChart(Gui2d& g, Samples const& s, float x, float width, ChartStyle const& st) {
    float const bottom = g.height();
    float const top    = bottom - 60.f;
    g.fill(x, top, x + width, bottom, 0x90505050);
    std::size_t const n     = std::min<std::size_t>(s.v.size(), static_cast<std::size_t>(width));
    double            minV  = 1e18, maxV = 0, sum = 0;
    for (std::size_t i = 0; i < n; ++i) {
        double const v  = s.v[s.v.size() - n + i];
        minV            = std::min(minV, v);
        maxV            = std::max(maxV, v);
        sum += v;
        float const h   = static_cast<float>(std::min(60.0, v * 60.0 / st.fullHeightValue));
        float const bx  = x + static_cast<float>(i);
        g.fill(bx, bottom - h, bx + 1.f, bottom, sampleColor(v, st.colorMid, st.colorHigh));
    }
    g.fill(x, top, x + width, top + 1.f, 0xFFFFFFFF);
    g.fill(x, top, x + 1.f, bottom, 0xFFFFFFFF);
    g.fill(x + width - 1.f, top, x + width, bottom, 0xFFFFFFFF);
    for (auto const& [v, label] : st.lines) {
        float const ly = bottom - static_cast<float>(v * 60.0 / st.fullHeightValue);
        if (ly < top) continue;
        g.fill(x + 1.f, ly, x + width - 1.f, ly + 1.f, 0xFFFFFFFF);
        g.text(Component::literal(label), x + 2.f, ly + 2.f, 0xFFE0E0E0, true, 0.75f);
    }
    if (n > 0) {
        Component const a = Component::literal(st.fmt(minV) + " min");
        Component const b = Component::literal(st.fmt(sum / static_cast<double>(n)) + " avg");
        Component const c = Component::literal(st.fmt(maxV) + " max");
        float const     ty = top - 9.f;
        g.text(a, x + 2.f, ty, 0xFFE0E0E0, true);
        g.text(b, x + width / 2.f - g.width(b) / 2.f, ty, 0xFFE0E0E0, true);
        g.text(c, x + width - g.width(c) - 2.f, ty, 0xFFE0E0E0, true);
    }
}

void collect() {
    auto const now = std::chrono::steady_clock::now();
    if (gLastFrame.time_since_epoch().count() != 0) {
        gFrame.push(static_cast<float>(std::chrono::duration<double, std::milli>(now - gLastFrame).count()));
    }
    gLastFrame = now;
    auto& pl       = ProfilerLite::gProfilerLiteInstance();
    auto  tickNs   = static_cast<std::int64_t>(pl.mDebugServerTickTime.get().count());
    if (tickNs > 0 && tickNs != gLastTickNs) gTick.push(static_cast<float>(tickNs / 1e6));
    gLastTickNs = tickNs;
}

}

void onFrame(Gui2d& g) {
    collect();
    if (GeneralOptions::disableMod || !DebugOverlay::isOverlayVisible) return;
    float const half = std::min(g.width() / 2.f, static_cast<float>(kMaxSamples));
    if (DebugOverlay::renderFpsCharts) {
        drawChart(g, gFrame, 0.f, half, ChartStyle{1000.0 / 30.0, 1000.0 / 60.0, 1000.0 / 15.0, {{1000.0 / 60.0, "60 FPS"}, {1000.0 / 30.0, "30 FPS"}}, ms});
        drawChart(g, gTick, g.width() - half, half, ChartStyle{50.0, 25.0, 50.0, {{50.0, "20 TPS"}}, ms});
    } else if (DebugOverlay::renderNetworkCharts) {
        auto kib = [](double v) {
            char b[32];
            std::snprintf(b, sizeof b, "%.1f KiB/s", v / 1024.0);
            return std::string(b);
        };
        drawChart(g, gPing, 0.f, half, ChartStyle{500.0, 100.0, 300.0, {{100.0, "100 ms"}, {250.0, "250 ms"}}, ms});
        drawChart(g, gBandwidth, g.width() - half, half, ChartStyle{1024.0 * 100.0, 1024.0 * 10.0, 1024.0 * 100.0, {{1024.0, "1 KiB/s"}, {10240.0, "10 KiB/s"}}, kib});
    }
    if (DebugOverlay::renderProfilerChart) drawPie(g);
}

bool onDigit(int digit) {
    if (GeneralOptions::disableMod || !DebugOverlay::isOverlayVisible || !DebugOverlay::renderProfilerChart) return false;
    if (digit == 0) {
        if (gPath.size() > 1) gPath.pop_back();
    } else if (digit >= 1 && digit <= 9) {
        ProfilerLite::ScopedData const* n = nullptr;
        int                             k = 0;
        for (auto const& e : gEntries) {
            if (e.node && ++k == digit) {
                n = e.node;
                break;
            }
        }
        if (!n) return true;
        gPath.push_back(n);
    } else {
        return false;
    }
    gLastPieUpdate = 0;
    gLastInclusive.clear();
    gEntries.clear();
    return true;
}

void pushPing(float v) { gPing.push(v); }
void pushTick(float v) { gTick.push(v); }
void pushBandwidth(float v) { gBandwidth.push(v); }

std::string profilerPath() {
    std::string p = "root";
    for (size_t i = 1; i < gPath.size(); ++i) p += std::string(".") + (gPath[i]->mName ? gPath[i]->mName : "?");
    return p;
}

std::vector<std::string> profilerEntries() {
    std::vector<std::string> out;
    for (auto const& e : gEntries) out.push_back(e.name + " " + pct(e.percent));
    return out;
}

std::size_t frameSamples() { return gFrame.v.size(); }
std::size_t tickSamples() { return gTick.v.size(); }

}
