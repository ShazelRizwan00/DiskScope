#include "ui/Dashboard.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include <imgui.h>
#include <implot.h>

#include "services/AnalysisEngine.h"
#include "services/ScanController.h"
#include "ui/AppState.h"
#include "ui/Theme.h"
#include "utils/Formatting.h"
#include "visualization/ChartRenderer.h"

namespace diskscope {

namespace {

void drawProgressCard(const AppState& state) {
    ScanProgress prog{};
    if (state.controller) {
        prog = state.controller->progress();
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::colors().cardV);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::colors().borderV);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    if (ImGui::BeginChild("ProgressCard", ImVec2(0.0f, 76.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar)) {
        ImGui::TextColored(theme::colors().accentV, "Scan in progress");
        ImGui::SameLine();
        const std::string counters = format::count(prog.folders) + " folders · "
                                   + format::count(prog.files) + " files · "
                                   + format::byteSize(prog.bytes);
        ImGui::TextColored(theme::colors().textMutedV, "·  %s", counters.c_str());

        if (!prog.current.empty()) {
            const std::string curr = format::ellipsize(prog.current.string(), 75);
            ImGui::TextColored(theme::colors().textMutedV, "%s", curr.c_str());
        } else {
            ImGui::TextColored(theme::colors().textMutedV, "Traversing directory hierarchy...");
        }

        const float fraction = -1.0f * static_cast<float>(ImGui::GetTime());
        const char* overlay = (prog.bytes == 0) ? "Discovering entries..." : "Scanning...";
        ImGui::ProgressBar(fraction, ImVec2(-1.0f, 14.0f), overlay);
    }
    ImGui::EndChild();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

}  // namespace

void Dashboard::updateCache(AppState& state) {
    if (!state.engine || !state.engine->hasResult()) {
        cachedGeneration_ = 0;
        cachedFolderCount_ = 0;
        cachedTopFolders_.clear();
        barXs_.clear();
        barYs_.clear();
        barTickPositions_.clear();
        barTickLabels_.clear();
        barTickLabelPtrs_.clear();
        pieValues_.clear();
        pieLabels_.clear();
        pieLabelPtrs_.clear();
        return;
    }

    cachedGeneration_ = state.engine->generation();
    cachedTopFolders_ = state.engine->topK(8);

    barXs_.clear();
    barYs_.clear();
    barTickPositions_.clear();
    barTickLabels_.clear();
    barTickLabelPtrs_.clear();
    barMaxX_ = 1.0;

    const std::size_t n = cachedTopFolders_.size();
    if (n >= 2) {
        barXs_.resize(n);
        barYs_.resize(n);
        barTickPositions_.resize(n);
        barTickLabels_.resize(n);
        barTickLabelPtrs_.resize(n);

        for (std::size_t i = 0; i < n; ++i) {
            const FolderNode* node = cachedTopFolders_[i];
            const double mb = node ? (static_cast<double>(node->totalBytes) / (1024.0 * 1024.0)) : 0.0;
            if (mb > barMaxX_) {
                barMaxX_ = mb;
            }
            const std::size_t slot = n - 1 - i;
            barXs_[slot] = mb;
            barYs_[slot] = static_cast<double>(slot);
            barTickPositions_[slot] = static_cast<double>(slot);
            barTickLabels_[slot] = node ? format::ellipsize(node->name, 16) : std::string();
        }
        for (std::size_t i = 0; i < n; ++i) {
            barTickLabelPtrs_[i] = barTickLabels_[i].c_str();
        }
    }

    pieValues_.clear();
    pieLabels_.clear();
    pieLabelPtrs_.clear();

    const AnalysisStats stats = state.engine->stats();
    // Snapshot of the ranked folder count so the cache is also invalidated when a
    // rescan changes how many folders exist. Comparing this against the top-k
    // size instead would rebuild the cache on every single frame.
    cachedFolderCount_ = static_cast<std::size_t>(stats.indexedFolders);
    if (n >= 2 && stats.totalBytes > 0) {
        const std::size_t topPieCount = std::min<std::size_t>(n, 5);
        std::uintmax_t topPieBytes = 0;
        for (std::size_t i = 0; i < topPieCount; ++i) {
            if (cachedTopFolders_[i]) {
                topPieBytes += cachedTopFolders_[i]->totalBytes;
            }
        }

        const bool hasOther = (stats.totalBytes > topPieBytes) && (stats.folderCount > topPieCount);
        const std::uintmax_t otherBytes = hasOther ? (stats.totalBytes - topPieBytes) : 0;
        const std::size_t totalSlices = topPieCount + (hasOther && otherBytes > 0 ? 1 : 0);

        pieValues_.reserve(totalSlices);
        pieLabels_.reserve(totalSlices);

        for (std::size_t i = 0; i < topPieCount; ++i) {
            const FolderNode* node = cachedTopFolders_[i];
            const std::uintmax_t b = node ? node->totalBytes : 0;
            const double frac = static_cast<double>(b) / static_cast<double>(stats.totalBytes);
            const double pct = frac * 100.0;
            pieValues_.push_back(pct);
            const std::string name = node ? node->name : "Unknown";
            pieLabels_.push_back(format::ellipsize(name, 14) + " (" + format::percent(frac) + ")");
        }

        if (hasOther && otherBytes > 0) {
            const double frac = static_cast<double>(otherBytes) / static_cast<double>(stats.totalBytes);
            const double pct = frac * 100.0;
            pieValues_.push_back(pct);
            pieLabels_.push_back("Other (" + format::percent(frac) + ")");
        }

        pieLabelPtrs_.resize(pieLabels_.size());
        for (std::size_t i = 0; i < pieLabels_.size(); ++i) {
            pieLabelPtrs_[i] = pieLabels_[i].c_str();
        }
    }
}

void Dashboard::draw(AppState& state) {
    if (state.scanRunning) {
        drawProgressCard(state);
        ImGui::Spacing();
    }

    if (!state.engine || !state.engine->hasResult()) {
        if (!state.scanRunning) {
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            if (avail.y > 140.0f) {
                ImGui::Dummy(ImVec2(0.0f, (avail.y - 100.0f) * 0.35f));
            }
            ImGui::PushFont(theme::heading());
            const char* headingText = "No scan loaded";
            const float hWidth = ImGui::CalcTextSize(headingText).x;
            if (avail.x > hWidth) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - hWidth) * 0.5f);
            }
            ImGui::TextUnformatted(headingText);
            ImGui::PopFont();

            ImGui::Spacing();
            const char* subtitle = "Choose a folder above and press Scan to analyze disk space.";
            const float sWidth = ImGui::CalcTextSize(subtitle).x;
            if (avail.x > sWidth) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - sWidth) * 0.5f);
            }
            ImGui::TextColored(theme::colors().textMutedV, "%s", subtitle);
        }
        return;
    }

    const AnalysisStats stats = state.engine->stats();

    if (cachedGeneration_ != state.engine->generation() || cachedTopFolders_.empty() || cachedFolderCount_ != stats.indexedFolders) {
        updateCache(state);
    }

    // Banner notifications
    if (stats.cancelled) {
        ImGui::TextColored(theme::colors().warnV, "Scan cancelled — partial results, totals are lower bounds");
        ImGui::Spacing();
    }
    if (stats.errorCount > 0) {
        ImGui::TextColored(theme::colors().warnV, "%s unreadable %s encountered during scan",
                           format::count(stats.errorCount).c_str(),
                           stats.errorCount == 1 ? "entry" : "entries");
        ImGui::Spacing();
    }
    if (!stats.rootExcludedFromRankings) {
        ImGui::TextColored(theme::colors().textMutedV, "Note: Scan root is included in rankings");
        ImGui::Spacing();
    }

    // Summary row of 4 compact cards
    const float availW = ImGui::GetContentRegionAvail().x;
    const float spacing = 8.0f;
    const float cardW = (availW - 3.0f * spacing) / 4.0f;
    const float cardH = 82.0f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::colors().cardV);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::colors().borderV);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    // Card 1: Scanned storage
    if (ImGui::BeginChild("CardStorage", ImVec2(cardW, cardH), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar)) {
        ImGui::TextColored(theme::colors().textMutedV, "Scanned storage");
        ImGui::PushFont(theme::heading());
        ImGui::TextUnformatted(format::byteSize(stats.totalBytes).c_str());
        ImGui::PopFont();
        ImGui::TextColored(theme::colors().textMutedV, "%s on disk", format::byteSize(stats.totalAllocatedBytes).c_str());
    }
    ImGui::EndChild();

    ImGui::SameLine(0.0f, spacing);

    // Card 2: Files
    if (ImGui::BeginChild("CardFiles", ImVec2(cardW, cardH), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar)) {
        ImGui::TextColored(theme::colors().textMutedV, "Files");
        ImGui::PushFont(theme::heading());
        ImGui::TextUnformatted(format::count(stats.fileCount).c_str());
        ImGui::PopFont();
        ImGui::TextColored(theme::colors().textMutedV, "regular files");
    }
    ImGui::EndChild();

    ImGui::SameLine(0.0f, spacing);

    // Card 3: Folders
    if (ImGui::BeginChild("CardFolders", ImVec2(cardW, cardH), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar)) {
        ImGui::TextColored(theme::colors().textMutedV, "Folders");
        ImGui::PushFont(theme::heading());
        ImGui::TextUnformatted(format::count(stats.folderCount).c_str());
        ImGui::PopFont();
        ImGui::TextColored(theme::colors().textMutedV, "%s indexed in heap", format::count(stats.indexedFolders).c_str());
    }
    ImGui::EndChild();

    ImGui::SameLine(0.0f, spacing);

    // Card 4: Largest folder
    if (ImGui::BeginChild("CardLargest", ImVec2(cardW, cardH), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar)) {
        ImGui::TextColored(theme::colors().textMutedV, "Largest folder");
        if (stats.largestFolder) {
            ImGui::PushFont(theme::heading());
            ImGui::TextUnformatted(format::byteSize(stats.largestFolder->totalBytes).c_str());
            ImGui::PopFont();
            ImGui::TextColored(theme::colors().textMutedV, "%s", format::ellipsize(stats.largestFolder->name, 18).c_str());
        } else {
            ImGui::PushFont(theme::heading());
            ImGui::TextUnformatted("None");
            ImGui::PopFont();
            ImGui::TextColored(theme::colors().textMutedV, "no subfolders");
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    // Analytics section
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (cachedTopFolders_.size() < 2) {
        ImGui::TextColored(theme::colors().textMutedV, "Charts require at least 2 folders in scan results.");
    } else {
        const float fullW = ImGui::GetContentRegionAvail().x;
        const float chartH = 260.0f;
        const float barW = (fullW > 680.0f) ? (fullW * 0.55f) : fullW;
        const float pieW = (fullW > 680.0f) ? (fullW - barW - 12.0f) : fullW;

        if (ImPlot::BeginPlot("Largest folders##BarPlot", ImVec2(barW, chartH), ImPlotFlags_NoMouseText)) {
            viz::applyPlotStyle();
            ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_None);
            ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoGridLines);
            ImPlot::SetupAxisFormat(ImAxis_X1, viz::byteAxisTickMb);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, barMaxX_ * 1.15, ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -0.6, static_cast<double>(barXs_.size()) - 0.4, ImPlotCond_Always);
            ImPlot::SetupAxisTicks(ImAxis_Y1, barTickPositions_.data(), static_cast<int>(barTickPositions_.size()), barTickLabelPtrs_.data());
            ImPlot::SetNextFillStyle(theme::colors().accentV, 0.75f);
            ImPlot::PlotBars("##Bars", barXs_.data(), barYs_.data(), static_cast<int>(barXs_.size()), 0.6, ImPlotBarsFlags_Horizontal);
            ImPlot::EndPlot();
        }

        if (fullW > 680.0f) {
            ImGui::SameLine(0.0f, 12.0f);
        } else {
            ImGui::Spacing();
        }

        if (pieValues_.size() >= 2) {
            if (ImPlot::BeginPlot("Storage share##PiePlot", ImVec2(pieW, chartH), ImPlotFlags_Equal | ImPlotFlags_NoMouseText)) {
                viz::applyPlotStyle();
                ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                ImPlot::SetupAxesLimits(0.0, 1.0, 0.0, 1.0);
                ImPlot::PlotPieChart(pieLabelPtrs_.data(), pieValues_.data(), static_cast<int>(pieValues_.size()), 0.5, 0.5, 0.38, "%.1f%%", 90.0, ImPlotPieChartFlags_Normalize);
                ImPlot::EndPlot();
            }
        }
    }

    // Footer line
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const std::string rootPathStr = state.scanRoot.empty() ? "(none)" : state.scanRoot.string();
    std::string footer = "Scan root: " + format::ellipsize(rootPathStr, 65);
    if (stats.rootExcludedFromRankings) {
        footer += "  ·  Rankings exclude the scan root";
    }
    ImGui::TextColored(theme::colors().textMutedV, "%s", footer.c_str());
}

}  // namespace diskscope
