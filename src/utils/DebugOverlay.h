// SPDX-License-Identifier: MIT
#pragma once

namespace better_f3::DebugOverlay {

inline bool isOverlayVisible    = false;
inline bool renderProfilerChart = false;
inline bool renderFpsCharts     = false;
inline bool renderNetworkCharts = false;

void setOverlayVisible(bool visible);
inline void toggleOverlay() { setOverlayVisible(!isOverlayVisible); }

void reset();

}
