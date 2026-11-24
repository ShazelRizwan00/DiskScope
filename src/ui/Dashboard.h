#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/FolderNode.h"
#include "ui/AppState.h"
#include "ui/Icons.h"

namespace diskscope {

struct AnalysisStats;

// Overview screen: scan status, headline metrics and the two storage charts.
//
// All figures come from AnalysisEngine. Everything derived from a scan result is
// cached per generation, because topK() and search() allocate and the view is
// redrawn every frame.
class Dashboard {
public:
    Dashboard() = default;
    void draw(AppState& state);

private:
    void updateCache(AppState& state);
    void drawScanCard(const AppState& state);
    void drawStatCards(const AnalysisStats& stats, float width);
    void drawCharts(float width);
    void drawBarChart(float height);
    void drawDonutCard(float height);
    void drawBanner(ui::Icon icon, bool danger, const char* text);

    std::uint64_t cachedGeneration_ = 0;
    std::size_t cachedFolderCount_ = 0;
    std::vector<const FolderNode*> cachedTopFolders_;

    // Bar chart: horizontal bars of the top folders, in megabytes.
    std::vector<double> barXs_;
    std::vector<double> barYs_;
    std::vector<double> barTickPositions_;
    std::vector<std::string> barTickLabels_;
    std::vector<const char*> barTickLabelPtrs_;
    double barMaxX_ = 1.0;

    // Donut: whole bytes per slice so the legend can print absolute sizes too.
    std::vector<double> donutValues_;
    std::vector<ImU32> donutColours_;
    std::vector<std::string> donutNames_;
    std::vector<std::string> donutSizes_;
    std::vector<std::string> donutShares_;
    std::uintmax_t donutTotal_ = 0;
};

}  // namespace diskscope
