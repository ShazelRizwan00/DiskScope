#include "visualization/ChartRenderer.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#include <implot.h>

#include "ui/Theme.h"
#include "utils/Formatting.h"

namespace diskscope::viz {

int byteAxisTickMb(double valueMegabytes, char* buffer, int size, void* userData) {
    (void)userData;
    if (buffer == nullptr || size <= 0) {
        return 0;
    }
    if (valueMegabytes <= 0.0) {
        return std::snprintf(buffer, static_cast<std::size_t>(size), "0 B");
    }
    const auto bytes = static_cast<std::uintmax_t>(valueMegabytes * 1024.0 * 1024.0);
    const std::string text = format::byteSizeCompact(bytes);
    return std::snprintf(buffer, static_cast<std::size_t>(size), "%s", text.c_str());
}

void applyPlotStyle() {
    // Safe to call before initialisation: there is nothing to style yet.
    if (ImPlot::GetCurrentContext() == nullptr) {
        return;
    }

    const theme::Colors& colors = theme::colors();
    ImPlotStyle& style = ImPlot::GetStyle();

    style.PlotBorderSize = 1.0f;
    style.PlotPadding = ImVec2(10.0f, 10.0f);
    style.LabelPadding = ImVec2(5.0f, 5.0f);
    style.LegendPadding = ImVec2(6.0f, 6.0f);
    style.LegendInnerPadding = ImVec2(4.0f, 4.0f);
    style.MinorAlpha = 0.25f;

    style.Colors[ImPlotCol_FrameBg] = colors.cardV;
    style.Colors[ImPlotCol_PlotBg] = colors.backgroundV;
    style.Colors[ImPlotCol_PlotBorder] = colors.borderV;
    style.Colors[ImPlotCol_LegendBg] = colors.cardV;
    style.Colors[ImPlotCol_LegendText] = colors.textV;
    style.Colors[ImPlotCol_AxisText] = colors.textMutedV;
    style.Colors[ImPlotCol_AxisGrid] = ImVec4(colors.borderV.x, colors.borderV.y, colors.borderV.z, 0.55f);
    style.Colors[ImPlotCol_AxisTick] = colors.textMutedV;
}

}  // namespace diskscope::viz
