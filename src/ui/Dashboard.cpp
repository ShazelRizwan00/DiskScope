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
#include "ui/Components.h"
#include "ui/Icons.h"
#include "ui/Theme.h"
#include "utils/Formatting.h"
#include "visualization/ChartRenderer.h"

namespace diskscope {
namespace {

// The charts are wide rather than tall: two side by side on a normal window,
// stacked once the window gets narrow.
constexpr float kSideBySideWidth = 880.0f;
constexpr float kChartHeight = 264.0f;
constexpr float kFourCardsWidth = 860.0f;

// Tonal green ramp for the ring leaders. Blending the two palette greens keeps
// every slice inside the brand family instead of reaching for red, which would
// read as an error state.
ImU32 ringColour(std::size_t index, std::size_t count) {
    const ImVec4 a = theme::colors().greenV;
    const ImVec4 b = theme::colors().greenDarkV;
    const float t = (count <= 1) ? 0.0f : static_cast<float>(index) / static_cast<float>(count - 1);
    const ImVec4 mixed(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, 1.0f);
    return ImGui::ColorConvertFloat4ToU32(mixed);
}

}  // namespace

void Dashboard::updateCache(AppState& state) {
    cachedTopFolders_.clear();
    barXs_.clear();
    barYs_.clear();
    barTickPositions_.clear();
    barTickLabels_.clear();
    barTickLabelPtrs_.clear();
    donutValues_.clear();
    donutColours_.clear();
    donutNames_.clear();
    donutSizes_.clear();
    donutShares_.clear();
    donutTotal_ = 0;
    barMaxX_ = 0.0;

    if (!state.engine || !state.engine->hasResult()) {
        cachedGeneration_ = 0;
        cachedFolderCount_ = 0;
        return;
    }

    cachedGeneration_ = state.engine->generation();
    const AnalysisStats stats = state.engine->stats();
    cachedFolderCount_ = static_cast<std::size_t>(stats.indexedFolders);
    cachedTopFolders_ = state.engine->topK(8);

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
            barMaxX_ = (std::max)(barMaxX_, mb);
            // Slot 0 is drawn at the bottom, so the largest folder reads first.
            const std::size_t slot = n - 1 - i;
            barXs_[slot] = mb;
            barYs_[slot] = static_cast<double>(slot);
            barTickPositions_[slot] = static_cast<double>(slot);
            barTickLabels_[slot] = node ? format::ellipsize(node->name, 18) : std::string();
        }
        for (std::size_t i = 0; i < n; ++i) {
            barTickLabelPtrs_[i] = barTickLabels_[i].c_str();
        }
    }

    if (n >= 2 && stats.totalBytes > 0) {
        // The ranking heap lists folders at every depth, so the top few overlap
        // (a parent contains its children) and would sum past 100%. A share chart
        // must be a true partition, so the ring uses the root's direct children
        // plus an explicit remainder and the root's own files.
        const FolderNode* root = state.engine->result().root.get();
        std::vector<const FolderNode*> children;
        if (root != nullptr) {
            children.reserve(root->children.size());
            for (const auto& child : root->children) {
                children.push_back(child.get());
            }
            std::sort(children.begin(), children.end(), [](const FolderNode* lhs, const FolderNode* rhs) {
                if (lhs->totalBytes != rhs->totalBytes) {
                    return lhs->totalBytes > rhs->totalBytes;
                }
                return lhs->name < rhs->name;
            });
        }

        donutTotal_ = stats.totalBytes;
        const std::size_t leaders = (std::min<std::size_t>)(children.size(), 5);
        for (std::size_t i = 0; i < leaders; ++i) {
            const FolderNode* node = children[i];
            if (node->totalBytes == 0) {
                continue;  // an empty folder is not a slice, it is noise in the legend
            }
            const double share = static_cast<double>(node->totalBytes) / static_cast<double>(stats.totalBytes);
            donutValues_.push_back(static_cast<double>(node->totalBytes));
            donutNames_.push_back(format::ellipsize(node->name, 18));
            donutSizes_.push_back(format::byteSize(node->totalBytes));
            donutShares_.push_back(format::percent(share));
        }

        std::uintmax_t tailBytes = 0;
        for (std::size_t i = leaders; i < children.size(); ++i) {
            tailBytes += children[i]->totalBytes;
        }
        if (tailBytes > 0) {
            const double share = static_cast<double>(tailBytes) / static_cast<double>(stats.totalBytes);
            donutValues_.push_back(static_cast<double>(tailBytes));
            donutNames_.push_back("Other folders");
            donutSizes_.push_back(format::byteSize(tailBytes));
            donutShares_.push_back(format::percent(share));
        }

        // Files sitting directly in the scan root belong to no child folder, so
        // they are their own slice rather than silently unaccounted for.
        if (root != nullptr && root->ownFileBytes > 0) {
            const double share =
                static_cast<double>(root->ownFileBytes) / static_cast<double>(stats.totalBytes);
            donutValues_.push_back(static_cast<double>(root->ownFileBytes));
            donutNames_.push_back("Files in scan root");
            donutSizes_.push_back(format::byteSize(root->ownFileBytes));
            donutShares_.push_back(format::percent(share));
        }

        // Leaders share the tonal green ramp; the two remainder slices are neutral
        // so they read as "everything else" rather than as another leader.
        const std::size_t leaderSlices = leaders;
        donutColours_.reserve(donutValues_.size());
        for (std::size_t i = 0; i < donutValues_.size(); ++i) {
            if (i < leaderSlices) {
                donutColours_.push_back(ringColour(i, leaderSlices));
            } else {
                donutColours_.push_back(i + 1 == donutValues_.size() ? theme::colors().borderStrong
                                                                     : theme::colors().textSecondary);
            }
        }
    }
}

void Dashboard::drawBanner(ui::Icon icon, bool danger, const char* text) {
    const theme::Colors& colors = theme::colors();
    const ImU32 ink = danger ? colors.red : colors.textSecondary;

    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float lineH = ImGui::GetTextLineHeight();
    ui::DrawIcon(ImGui::GetWindowDrawList(), icon, ImVec2(start.x + 8.0f, start.y + lineH * 0.5f), 16.0f,
                 ink, 1.6f);
    ImGui::Dummy(ImVec2(20.0f, lineH));
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::TextColored(danger ? colors.redV : colors.textSecondaryV, "%s", text);
}

void Dashboard::drawScanCard(const AppState& state) {
    ScanProgress progress{};
    if (state.controller) {
        progress = state.controller->progress();
    }

    const ui::CardOptions options{"Scan in progress", nullptr, ui::Icon::Scan};
    if (ui::BeginCard("DashboardScanCard", options)) {
        char counters[256];
        std::snprintf(counters, sizeof(counters), "%s folders  |  %s files  |  %s",
                      format::count(progress.folders).c_str(),
                      format::count(progress.files).c_str(),
                      format::byteSize(progress.bytes).c_str());
        ImGui::TextColored(theme::colors().textV, "%s", counters);

        const std::string current = progress.current.empty() ? std::string("Traversing the directory hierarchy...")
                                                            : format::ellipsize(progress.current.string(), 96);
        ImGui::PushFont(theme::caption());
        ImGui::TextColored(theme::colors().textSecondaryV, "%s", current.c_str());
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        ui::ProgressBar(-1.0f, ui::Tone::Green, 6.0f, nullptr);
    }
    ui::EndCard();
}

void Dashboard::drawStatCards(const AnalysisStats& stats, float width) {
    const theme::Metrics& metrics = theme::metrics();
    const int perRow = (width >= kFourCardsWidth) ? 4 : 2;
    const float cardW = (width - static_cast<float>(perRow - 1) * metrics.gap) / static_cast<float>(perRow);
    const ImVec2 size(cardW, 96.0f);

    const double largestShare = (stats.totalBytes > 0 && stats.largestFolder)
                                    ? (static_cast<double>(stats.largestFolder->totalBytes) /
                                       static_cast<double>(stats.totalBytes))
                                    : 0.0;

    struct Card {
        const char* id;
        ui::Icon icon;
        const char* label;
        std::string value;
        std::string sub;
        ui::Tone tone;
    };

    const std::string storage = format::byteSize(stats.totalBytes);
    const std::string allocated = format::byteSize(stats.totalAllocatedBytes) + " on disk";
    const std::string files = format::count(stats.fileCount);
    const std::string folders = format::count(stats.folderCount);
    const std::string indexed = format::count(stats.indexedFolders) + " indexed in heap";
    const std::string largestValue = stats.largestFolder ? format::byteSize(stats.largestFolder->totalBytes)
                                                         : std::string("None");
    const std::string largestSub = stats.largestFolder
                                       ? (format::ellipsize(stats.largestFolder->name, 20) + "  |  " +
                                          format::percent(largestShare) + " of scan")
                                       : std::string("no subfolders");

    const Card cards[4] = {
        {"##cardStorage", ui::Icon::Disk, "Scanned storage", storage, allocated, ui::Tone::Green},
        {"##cardFiles", ui::Icon::File, "Files", files, "regular files", ui::Tone::Neutral},
        {"##cardFolders", ui::Icon::Folder, "Folders", folders, indexed, ui::Tone::Neutral},
        // A folder holding half of everything is the one number worth flagging.
        {"##cardLargest", ui::Icon::Chart, "Largest folder", largestValue, largestSub,
         largestShare >= 0.5 ? ui::Tone::Red : ui::Tone::Neutral},
    };

    for (int i = 0; i < 4; ++i) {
        if (i % perRow != 0) {
            ImGui::SameLine(0.0f, metrics.gap);
        }
        ui::StatCard(cards[i].id, cards[i].icon, cards[i].label, cards[i].value.c_str(),
                     cards[i].sub.c_str(), cards[i].tone, size);
    }
}

void Dashboard::drawBarChart(float height) {
    const ui::CardOptions options{"Largest folders", "Inclusive size, largest first", ui::Icon::Chart,
                                  height};
    if (ui::BeginCard("DashboardBarCard", options)) {
        const ImVec2 available = ImGui::GetContentRegionAvail();
        if (available.y > 40.0f && !barXs_.empty()) {
            // Five evenly spaced byte ticks: ImPlot's own tick sweep produced 20
            // labels across the axis, which is unreadable at card width.
            constexpr int kTicks = 5;
            double tickPos[kTicks];
            std::string tickText[kTicks];
            const char* tickPtr[kTicks];
            const double axisMax = barMaxX_ * 1.12;
            for (int i = 0; i < kTicks; ++i) {
                const double fraction = static_cast<double>(i) / static_cast<double>(kTicks - 1);
                tickPos[i] = axisMax * fraction;
                tickText[i] = format::byteSizeCompact(
                    static_cast<std::uintmax_t>(tickPos[i] * 1024.0 * 1024.0));
                tickPtr[i] = tickText[i].c_str();
            }

            if (ImPlot::BeginPlot("##LargestFoldersPlot", ImVec2(available.x, available.y - 4.0f),
                                  ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend)) {
                viz::applyPlotStyle();
                ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoGridLines);
                ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks);
                ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, axisMax, ImPlotCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, -0.6, static_cast<double>(barXs_.size()) - 0.4,
                                        ImPlotCond_Always);
                ImPlot::SetupAxisTicks(ImAxis_X1, tickPos, kTicks, tickPtr);
                ImPlot::SetupAxisTicks(ImAxis_Y1, barTickPositions_.data(),
                                       static_cast<int>(barTickPositions_.size()), barTickLabelPtrs_.data());

                ImPlot::SetNextFillStyle(theme::colors().greenV, 0.9f);
                ImPlot::PlotBars("##Bars", barXs_.data(), barYs_.data(), static_cast<int>(barXs_.size()),
                                 0.66, ImPlotBarsFlags_Horizontal);
                ImPlot::EndPlot();
            }
        } else {
            ui::EmptyState(ui::Icon::Info, "No ranked folders", "Run a scan to populate the ranking.");
        }
    }
    ui::EndCard();
}

void Dashboard::drawDonutCard(float height) {
    const ui::CardOptions options{"Storage share", "Where the scanned bytes sit", ui::Icon::Layers, height};
    if (ui::BeginCard("DashboardDonutCard", options)) {
        const ImVec2 available = ImGui::GetContentRegionAvail();
        if (available.y < 60.0f || donutValues_.empty()) {
            ui::EmptyState(ui::Icon::Info, "Nothing to chart", "The ring needs at least two ranked folders.");
            ui::EndCard();
            return;
        }

        float ringSize = (std::min)(available.y, available.x * 0.5f);
        ringSize = (std::max)(ringSize, 90.0f);
        float legendW = available.x - ringSize - theme::metrics().gap;
        bool legendBeside = legendW >= 150.0f;
        if (!legendBeside) {
            ringSize = (std::min)(available.x, available.y * 0.7f);
        }

        const ImVec2 ringOrigin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##donutHit", ImVec2(ringSize, ringSize));
        const ImVec2 centre(ringOrigin.x + ringSize * 0.5f, ringOrigin.y + ringSize * 0.5f);

        ImDrawList* list = ImGui::GetWindowDrawList();
        const float outer = ringSize * 0.46f;
        const int hovered = viz::drawDonut(list, centre, outer, (std::max)(14.0f, outer * 0.34f),
                                           donutValues_, donutColours_, true);

        const std::string total = format::byteSize(donutTotal_);
        ImFont* totalFont = theme::h3();
        const ImVec2 totalSize = totalFont->CalcTextSizeA(totalFont->FontSize, 1000.0f, 0.0f, total.c_str());
        list->AddText(totalFont, totalFont->FontSize,
                      ImVec2(centre.x - totalSize.x * 0.5f, centre.y - totalSize.y * 0.5f - 7.0f),
                      theme::colors().text, total.c_str());

        ImFont* captionFont = theme::caption();
        const char* centreLabel = "scanned";
        const ImVec2 labelSize =
            captionFont->CalcTextSizeA(captionFont->FontSize, 1000.0f, 0.0f, centreLabel);
        list->AddText(captionFont, captionFont->FontSize,
                      ImVec2(centre.x - labelSize.x * 0.5f, centre.y + totalSize.y * 0.5f - 4.0f),
                      theme::colors().textSecondary, centreLabel);

        if (legendBeside) {
            ImGui::SameLine(0.0f, theme::metrics().gap);
        } else {
            ImGui::Dummy(ImVec2(0.0f, 4.0f));
            legendW = available.x;
        }

        ImGui::BeginGroup();
        const float rowH = 22.0f;
        const std::size_t rows = donutNames_.size();
        for (std::size_t i = 0; i < rows; ++i) {
            const ImVec2 rowStart = ImGui::GetCursorScreenPos();
            const bool isHovered = (static_cast<int>(i) == hovered);
            if (isHovered) {
                list->AddRectFilled(ImVec2(rowStart.x - 4.0f, rowStart.y),
                                    ImVec2(rowStart.x + legendW, rowStart.y + rowH),
                                    theme::colors().greenSoft, 4.0f);
            }

            list->AddRectFilled(ImVec2(rowStart.x, rowStart.y + rowH * 0.5f - 5.0f),
                                ImVec2(rowStart.x + 10.0f, rowStart.y + rowH * 0.5f + 5.0f),
                                donutColours_[i], 3.0f);

            ImGui::Dummy(ImVec2(16.0f, rowH));
            ImGui::SameLine(0.0f, 2.0f);
            ImGui::PushFont(isHovered ? theme::bodyStrong() : theme::body());
            ImGui::TextColored(isHovered ? theme::colors().greenV : theme::colors().textV, "%s",
                               donutNames_[i].c_str());
            ImGui::PopFont();

            // Right-aligned columns are drawn on the draw list: SameLine() offsets
            // are window-relative, so computing them from the cursor clipped the
            // share and size against the card edge.
            const std::string right = donutShares_[i] + "    " + donutSizes_[i];
            ImFont* rightFont = theme::caption();
            const ImVec2 rightSize =
                rightFont->CalcTextSizeA(rightFont->FontSize, 1000.0f, 0.0f, right.c_str());
            const float rightX = rowStart.x + legendW - rightSize.x;
            if (rightX > ImGui::GetCursorScreenPos().x + 8.0f) {
                list->AddText(rightFont, rightFont->FontSize,
                              ImVec2(rightX, rowStart.y + (rowH - rightSize.y) * 0.5f),
                              theme::colors().textSecondary, right.c_str());
            }
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (rowH - ImGui::GetTextLineHeight()) * 0.5f);
        }
        ImGui::EndGroup();
    }
    ui::EndCard();
}

void Dashboard::drawCharts(float width) {
    const theme::Metrics& metrics = theme::metrics();

    if (width >= kSideBySideWidth) {
        // A table drives the column widths: cards always fill their column, so the
        // split is decided here rather than by the card component.
        if (ImGui::BeginTable("##chartRow", 2,
                              ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoPadOuterX |
                                  ImGuiTableFlags_NoPadInnerX)) {
            ImGui::TableSetupColumn("##bar", ImGuiTableColumnFlags_WidthStretch, 1.35f);
            ImGui::TableSetupColumn("##donut", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            drawBarChart(kChartHeight);
            ImGui::TableNextColumn();
            drawDonutCard(kChartHeight);
            ImGui::EndTable();
        }
    } else {
        drawBarChart(kChartHeight);
        ImGui::Dummy(ImVec2(0.0f, metrics.gap));
        drawDonutCard(kChartHeight);
    }
}

void Dashboard::draw(AppState& state) {
    ImGui::BeginChild("DashboardScroll", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_None);

    ui::SectionHeader(ui::Icon::Overview, "Overview", "Storage summary for the scanned directory");

    if (state.scanRunning) {
        drawScanCard(state);
        ImGui::Dummy(ImVec2(0.0f, theme::metrics().gap));
    }

    if (!state.engine || !state.engine->hasResult()) {
        if (!state.scanRunning) {
            ui::EmptyState(ui::Icon::Disk, "No scan loaded",
                           "Choose a folder in the toolbar and press Scan to analyze disk space.");
        }
        ImGui::EndChild();
        return;
    }

    const AnalysisStats stats = state.engine->stats();
    if (cachedGeneration_ != state.engine->generation() || cachedTopFolders_.empty() ||
        cachedFolderCount_ != static_cast<std::size_t>(stats.indexedFolders)) {
        updateCache(state);
    }

    if (stats.cancelled) {
        drawBanner(ui::Icon::XCircle, true, "Scan cancelled - totals are lower bounds over the folders visited.");
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
    }
    if (stats.errorCount > 0) {
        const std::string text = format::count(stats.errorCount) +
                                 (stats.errorCount == 1 ? " entry could not be read" : " entries could not be read");
        drawBanner(ui::Icon::AlertTriangle, true, text.c_str());
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
    }
    if (!stats.rootExcludedFromRankings) {
        drawBanner(ui::Icon::Info, false, "The scan root itself is ranked alongside its subfolders.");
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
    }

    const float width = ImGui::GetContentRegionAvail().x;
    drawStatCards(stats, width);

    ImGui::Dummy(ImVec2(0.0f, theme::metrics().gap));

    if (cachedTopFolders_.size() < 2) {
        const ui::CardOptions options{"Charts", nullptr, ui::Icon::Chart};
        if (ui::BeginCard("DashboardChartsEmpty", options)) {
            ui::EmptyState(ui::Icon::Info, "Not enough folders to chart",
                           "The charts need at least two ranked folders in the scan result.");
        }
        ui::EndCard();
    } else {
        drawCharts(width);
    }

    ImGui::Dummy(ImVec2(0.0f, theme::metrics().gap));

    const std::string rootPath = state.scanRoot.empty() ? std::string("(none)") : state.scanRoot.string();
    ImGui::PushFont(theme::caption());
    ImGui::TextColored(theme::colors().textSecondaryV, "Scan root: %s",
                       format::ellipsize(rootPath, 96).c_str());
    ImGui::PopFont();

    ImGui::EndChild();
}

}  // namespace diskscope
