#pragma once

#include "ui/AppState.h"

namespace diskscope {

// Settings screen: scan behaviour, ranking options, visualizer playback, the
// palette reference and the known limitations of the scanner.
class SettingsView {
public:
    SettingsView() = default;
    void draw(AppState& state);

private:
    void drawScanningCard(AppState& state);
    void drawRankingCard(AppState& state);
    void drawVisualizerCard(AppState& state);
    void drawPaletteCard();
    void drawLimitationsCard();
    void drawAboutCard(AppState& state);
};

}  // namespace diskscope
