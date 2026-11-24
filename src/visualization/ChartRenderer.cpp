#include "visualization/ChartRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include <imgui.h>
#include <implot.h>

#include "ui/Theme.h"
#include "utils/Formatting.h"

namespace diskscope::viz {

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

    style.Colors[ImPlotCol_FrameBg] = colors.panelV;
    style.Colors[ImPlotCol_PlotBg] = colors.bgV;
    style.Colors[ImPlotCol_PlotBorder] = colors.borderV;
    style.Colors[ImPlotCol_LegendBg] = colors.panelV;
    style.Colors[ImPlotCol_LegendBorder] = colors.borderV;
    style.Colors[ImPlotCol_LegendText] = colors.textV;
    style.Colors[ImPlotCol_TitleText] = colors.textV;
    style.Colors[ImPlotCol_InlayText] = colors.textSecondaryV;
    style.Colors[ImPlotCol_AxisText] = colors.textSecondaryV;
    style.Colors[ImPlotCol_AxisGrid] =
        ImVec4(colors.borderV.x, colors.borderV.y, colors.borderV.z, 0.55f);
    style.Colors[ImPlotCol_AxisTick] = colors.textSecondaryV;
    style.Colors[ImPlotCol_Selection] = colors.greenSoftV;
    style.Colors[ImPlotCol_Crosshairs] = colors.textSecondaryV;
    style.Colors[ImPlotCol_AxisBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImPlotCol_AxisBgHovered] = colors.elevatedV;
    style.Colors[ImPlotCol_AxisBgActive] = colors.greenSoftV;

    // Per-item slots. ImPlot's defaults (blue, orange, purple, cyan) belong to no
    // part of this palette, so every slot is assigned from the theme ramp; a chart
    // can therefore never leak a default colour. Together with the entries above
    // this covers every ImPlotCol_ entry ImPlot 0.17 defines.
    style.Colors[ImPlotCol_Line] = colors.greenV;
    style.Colors[ImPlotCol_Fill] = ImVec4(colors.greenV.x, colors.greenV.y, colors.greenV.z, 0.35f);
    style.Colors[ImPlotCol_MarkerOutline] = colors.greenHoverV;
    style.Colors[ImPlotCol_MarkerFill] = colors.greenV;
    style.Colors[ImPlotCol_ErrorBar] = colors.redV;
}

}  // namespace diskscope::viz

namespace diskscope::viz {
namespace {

constexpr float kTwoPi = 6.28318530718f;

int segmentUnderCursor(ImVec2 centre, float outerRadius, float thickness,
                       const std::vector<double>& values, double total) {
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) || total <= 0.0) {
        return -1;
    }

    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const ImVec2 d(mouse.x - centre.x, mouse.y - centre.y);
    const float distance = std::sqrt(d.x * d.x + d.y * d.y);
    if (distance < outerRadius - thickness - 2.0f || distance > outerRadius + 2.0f) {
        return -1;
    }

    // atan2 gives [-pi, pi]; the ring starts at -pi/2 and grows clockwise, so the
    // measured angle is shifted into [0, 2pi) before comparing with the buckets.
    float angle = std::atan2(d.y, d.x) + kTwoPi * 0.25f;
    if (angle < 0.0f) {
        angle += kTwoPi;
    }

    float swept = 0.0f;
    for (std::size_t i = 0; i < values.size(); ++i) {
        const float portion = static_cast<float>(values[i] / total) * kTwoPi;
        if (angle >= swept && angle < swept + portion) {
            return static_cast<int>(i);
        }
        swept += portion;
    }
    return -1;
}

}  // namespace

int drawDonut(ImDrawList* list, ImVec2 centre, float outerRadius, float thickness,
              const std::vector<double>& values, const std::vector<ImU32>& colours,
              bool interactive) {
    if (list == nullptr || values.empty() || outerRadius <= 0.0f || colours.size() != values.size()) {
        return -1;
    }

    double total = 0.0;
    for (double v : values) {
        total += (v > 0.0) ? v : 0.0;
    }
    if (total <= 0.0) {
        return -1;
    }

    const int hovered = interactive ? segmentUnderCursor(centre, outerRadius, thickness, values, total) : -1;

    const float radius = outerRadius - thickness * 0.5f;
    float start = -kTwoPi * 0.25f;

    for (std::size_t i = 0; i < values.size(); ++i) {
        const double share = (values[i] > 0.0) ? (values[i] / total) : 0.0;
        const float sweep = static_cast<float>(share) * kTwoPi;
        if (sweep <= 0.0001f) {
            continue;
        }

        const bool isHovered = (static_cast<int>(i) == hovered);
        // Leave a hairline between slices so adjacent greens stay distinguishable.
        const float gap = (values.size() > 1) ? 0.012f : 0.0f;
        const float a0 = start + gap;
        const float a1 = start + sweep - gap;
        const int segments = (std::max)(2, static_cast<int>(std::fabs(a1 - a0) / 0.08f));

        if (a1 > a0) {
            list->PathArcTo(centre, radius, a0, a1, segments);
            list->PathStroke(colours[i], 0, thickness + (isHovered ? 4.0f : 0.0f));
        }

        start += sweep;
    }

    return hovered;
}

}  // namespace diskscope::viz
