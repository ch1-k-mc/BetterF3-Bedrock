// SPDX-License-Identifier: MIT
#include "utils/DebugOverlay.h"

#include "config/GeneralOptions.h"
#include "utils/ServerSampler.h"
#include "utils/Utils.h"

namespace better_f3::DebugOverlay {

void setOverlayVisible(bool visible) {
    if (GeneralOptions::disableMod) {
        isOverlayVisible = visible;
        return;
    }
    if (GeneralOptions::enableAnimations) {
        if (isOverlayVisible && !visible && !utils::closingAnimation) {
            utils::closingAnimation = true;
        } else if (!isOverlayVisible && visible) {
            utils::closingAnimation = false;
            utils::xPos             = utils::START_X_POS;
            isOverlayVisible        = true;
        }
    } else {
        isOverlayVisible = visible;
    }
}

void reset() {
    ServerSampler::clear();
    renderProfilerChart = false;
    renderFpsCharts     = false;
    renderNetworkCharts = false;
    utils::closingAnimation = false;
    if (!GeneralOptions::disableMod && GeneralOptions::autoF3) {
        setOverlayVisible(true);
        if (GeneralOptions::alwaysEnableProfiler) renderProfilerChart = true;
        if (GeneralOptions::alwaysEnableTPS) renderFpsCharts = true;
        if (GeneralOptions::alwaysEnablePing) renderNetworkCharts = !renderNetworkCharts;
    }
}

}
