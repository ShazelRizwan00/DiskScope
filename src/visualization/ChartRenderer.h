#pragma once

#include <vector>

#include <imgui.h>

namespace diskscope::viz {

// Applies DiskScope's palette to the ImPlot style singleton. Idempotent, so it
// is safe to call whenever a chart is drawn.
void applyPlotStyle();

// Draws a ring chart of `values` around `centre` using `colours` (one per slice)
// and returns the index under the mouse (-1 when nothing is hovered). The caller
// owns the legend so it can align it with the rest of the layout.
int drawDonut(ImDrawList* list, ImVec2 centre, float outerRadius, float thickness,
              const std::vector<double>& values, const std::vector<ImU32>& colours,
              bool interactive);

}  // namespace diskscope::viz
