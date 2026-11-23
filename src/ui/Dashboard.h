#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/FolderNode.h"
#include "ui/AppState.h"

namespace diskscope {

class Dashboard {
public:
    Dashboard() = default;
    void draw(AppState& state);

private:
    void updateCache(AppState& state);

    std::uint64_t cachedGeneration_ = 0;
    std::size_t cachedFolderCount_ = 0;
    std::vector<const FolderNode*> cachedTopFolders_;

    // Bar chart cached data
    std::vector<double> barXs_;
    std::vector<double> barYs_;
    std::vector<double> barTickPositions_;
    std::vector<std::string> barTickLabels_;
    std::vector<const char*> barTickLabelPtrs_;
    double barMaxX_ = 1.0;

    // Pie chart cached data
    std::vector<double> pieValues_;
    std::vector<std::string> pieLabels_;
    std::vector<const char*> pieLabelPtrs_;
};

}  // namespace diskscope
