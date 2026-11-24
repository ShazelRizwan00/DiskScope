#include "ui/LargestFoldersView.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include <imgui.h>

#include "services/AnalysisEngine.h"
#include "ui/AppState.h"
#include "ui/Components.h"
#include "ui/Icons.h"
#include "ui/Theme.h"
#include "utils/Formatting.h"

namespace diskscope {
namespace {

constexpr float kRowHeight = 40.0f;
constexpr int kTopKChoices[3] = {5, 10, 20};
const char* const kTopKLabels[3] = {"Top 5", "Top 10", "Top 20"};

int indexForK(int k) {
    for (int i = 0; i < 3; ++i) {
        if (kTopKChoices[i] == k) {
            return i;
        }
    }
    return 1;
}

// Rank pill: the first three places are the ones a user actually looks for, so
// they carry the accent while the rest stay neutral.
void drawRankBadge(ImDrawList* list, ImVec2 topLeft, int rank) {
    const theme::Colors& colors = theme::colors();
    char text[16];
    std::snprintf(text, sizeof(text), "#%d", rank);

    ImFont* font = theme::caption();
    const ImVec2 size = font->CalcTextSizeA(font->FontSize, 1000.0f, 0.0f, text);
    const float w = 32.0f;
    const float h = 20.0f;
    const ImVec2 max(topLeft.x + w, topLeft.y + h);
    const bool leading = rank <= 3;

    list->AddRectFilled(topLeft, max, leading ? colors.greenDark : colors.elevated, 5.0f);
    list->AddRect(topLeft, max, leading ? colors.green : colors.border, 5.0f, 0, 1.0f);
    list->AddText(font, font->FontSize,
                  ImVec2(topLeft.x + (w - size.x) * 0.5f, topLeft.y + (h - size.y) * 0.5f),
                  leading ? colors.green : colors.textSecondary, text);
}

}  // namespace

void LargestFoldersView::drawControls(AppState& state, const std::string& summary) {
    const theme::Colors& colors = theme::colors();

    int choice = indexForK(state.topKChoice);
    if (ui::Segmented("##topK", kTopKLabels, 3, choice, 288.0f)) {
        state.topKChoice = kTopKChoices[choice];
    }

    // Widths must be measured after SameLine: Segmented leaves the cursor on a new
    // line, so the region there still spans the whole row.
    ImGui::SameLine(0.0f, theme::metrics().gap);
    const float searchW = (std::max)(180.0f, ImGui::GetContentRegionAvail().x - 185.0f);
    ui::SearchInput("##folderSearch", state.searchTerm, "Filter folders by name", searchW);

    ImGui::SameLine(0.0f, theme::metrics().gap);
    ImGui::PushFont(theme::caption());
    ImGui::TextColored(colors.textSecondaryV, "%s", summary.c_str());
    ImGui::PopFont();
}

void LargestFoldersView::drawTable(AppState& state, const std::vector<const FolderNode*>& rows,
                                   bool searching) {
    const theme::Colors& colors = theme::colors();
    const std::uintmax_t rootTotal =
        state.engine->result().root ? state.engine->result().root->totalBytes : 0;
    const std::filesystem::path scanRoot = state.engine->result().root
                                               ? state.engine->result().root->path
                                               : std::filesystem::path();

    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp |
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerH;

    if (!ImGui::BeginTable("##largestFoldersTable", 5, flags, ImVec2(0.0f, 0.0f))) {
        return;
    }

    ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 56.0f);
    ImGui::TableSetupColumn("Folder", ImGuiTableColumnFlags_WidthStretch, 0.42f);
    ImGui::TableSetupColumn("Usage", ImGuiTableColumnFlags_WidthStretch, 0.22f);
    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 108.0f);
    ImGui::TableSetupColumn("Share", ImGuiTableColumnFlags_WidthFixed, 74.0f);
    ImGui::TableSetupScrollFreeze(0, 1);

    // Header row drawn by hand so the numeric columns line up with their values;
    // ImGui's TableHeadersRow always left-aligns the label.
    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
    ImGui::TableSetColumnIndex(0);
    ImGui::TextColored(colors.textSecondaryV, "Rank");
    ImGui::TableSetColumnIndex(1);
    ImGui::TextColored(colors.textSecondaryV, "Folder");
    ImGui::TableSetColumnIndex(2);
    ImGui::TextColored(colors.textSecondaryV, "Usage");
    ImGui::TableSetColumnIndex(3);
    ui::TableHeaderCellRight("Size");
    ImGui::TableSetColumnIndex(4);
    ui::TableHeaderCellRight("Share");

    for (std::size_t i = 0; i < rows.size(); ++i) {
        const FolderNode* node = rows[i];
        if (node == nullptr) {
            continue;
        }

        ImGui::TableNextRow(ImGuiTableRowFlags_None, kRowHeight);
        ImGui::TableSetColumnIndex(0);

        const ImVec2 cellOrigin = ImGui::GetCursorScreenPos();
        const bool selected = (node->id == state.selectedId);

        ImGui::PushID(static_cast<int>(node->id));
        // The selectable spans every column so the whole row is one click target,
        // then the cell contents are drawn on top of its highlight.
        if (ImGui::Selectable("##row", selected, ImGuiSelectableFlags_SpanAllColumns,
                              ImVec2(0.0f, kRowHeight - 4.0f))) {
            state.selectedId = node->id;
            state.requestSection(Section::DirectoryExplorer);
        }
        const bool hovered = ImGui::IsItemHovered();
        if (hovered) {
            ui::Tooltip(node->path.string().c_str());
        }
        ImGui::PopID();

        drawRankBadge(ImGui::GetWindowDrawList(), ImVec2(cellOrigin.x, cellOrigin.y + 6.0f),
                      static_cast<int>(i) + 1);

        ImGui::TableSetColumnIndex(1);
        {
            const float width = ImGui::GetContentRegionAvail().x;
            const std::size_t chars = static_cast<std::size_t>((std::max)(8.0f, width / 7.0f));
            ImGui::PushFont(theme::bodyStrong());
            ImGui::TextColored(selected ? colors.greenV : colors.textV, "%s",
                               format::ellipsize(node->name, chars).c_str());
            ImGui::PopFont();

            std::string sub = format::relativePath(node->path, scanRoot);
            if (sub.empty()) {
                sub = node->path.string();
            }
            ImGui::PushFont(theme::caption());
            ImGui::TextColored(colors.textSecondaryV, "%s",
                               format::ellipsize(sub, static_cast<std::size_t>((std::max)(10.0f, width / 6.0f)))
                                   .c_str());
            ImGui::PopFont();
        }

        ImGui::TableSetColumnIndex(2);
        {
            const double share =
                (rootTotal > 0) ? static_cast<double>(node->totalBytes) / static_cast<double>(rootTotal)
                                : 0.0;
            const float meterW = ImGui::GetContentRegionAvail().x;
            // Vertically centre the meter against the two-line name cell.
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);
            ui::BarMeter(static_cast<float>(share), meterW, ui::Tone::Green, 6.0f);
        }

        ImGui::TableSetColumnIndex(3);
        {
            const std::string sizeText = format::byteSize(node->totalBytes);
            ImFont* mono = theme::mono();
            const ImVec2 size = mono->CalcTextSizeA(mono->FontSize, 1000.0f, 0.0f, sizeText.c_str());
            const ImVec2 start = ImGui::GetCursorScreenPos();
            const float right = start.x + ImGui::GetContentRegionAvail().x;
            ImGui::GetWindowDrawList()->AddText(mono, mono->FontSize,
                                                ImVec2(right - size.x, start.y + 2.0f), colors.text,
                                                sizeText.c_str());
            ImGui::Dummy(ImVec2(0.0f, size.y));
        }

        ImGui::TableSetColumnIndex(4);
        {
            const double share =
                (rootTotal > 0) ? static_cast<double>(node->totalBytes) / static_cast<double>(rootTotal)
                                : 0.0;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
            ImGui::PushFont(theme::body());
            ImGui::TextColored(colors.textSecondaryV, "%s", format::percent(share).c_str());
            ImGui::PopFont();
        }
    }

    ImGui::EndTable();
    (void)searching;
}

void LargestFoldersView::draw(AppState& state) {
    const theme::Colors& colors = theme::colors();

    if (!state.engine || !state.engine->hasResult()) {
        ui::SectionHeader(ui::Icon::Chart, "Largest Folders", "Ranked by inclusive folder size");
        ui::EmptyState(ui::Icon::Chart, "No scan loaded",
                       "Choose a folder in the toolbar and press Scan to rank its largest folders.");
        return;
    }

    const AnalysisStats stats = state.engine->stats();
    ui::SectionHeader(ui::Icon::Chart, "Largest Folders",
                      stats.rootExcludedFromRankings
                          ? "Ranked by inclusive size, scan root excluded"
                          : "Ranked by inclusive size, scan root included");

    const bool needRecompute = (cachedGeneration_ != state.engine->generation()) ||
                               (cachedK_ != static_cast<std::size_t>(state.topKChoice)) ||
                               (cachedIncludeRoot_ != state.includeRoot) ||
                               (cachedSearchTerm_ != state.searchTerm);
    if (needRecompute) {
        cachedGeneration_ = state.engine->generation();
        cachedK_ = static_cast<std::size_t>(state.topKChoice);
        cachedIncludeRoot_ = state.includeRoot;
        cachedSearchTerm_ = state.searchTerm;
        if (state.searchTerm.empty()) {
            cachedTopK_ = state.engine->topK(cachedK_);
            cachedSearchResults_.clear();
        } else {
            cachedSearchResults_ = state.engine->search(state.searchTerm, 200);
            cachedTopK_.clear();
        }
    }

    const bool searching = !state.searchTerm.empty();
    const std::vector<const FolderNode*>& rows = searching ? cachedSearchResults_ : cachedTopK_;

    std::string summary;
    if (searching) {
        summary = format::count(rows.size()) + " matches";
    } else {
        summary = format::count(stats.indexedFolders) + " folders ranked";
    }

    drawControls(state, summary);
    ImGui::Dummy(ImVec2(0.0f, theme::metrics().gap));

    if (searching && rows.empty()) {
        ui::EmptyState(ui::Icon::Search, "No matches",
                       "No folder name in this scan contains the filter text.");
        return;
    }
    if (!searching && stats.folderCount == 0) {
        ui::EmptyState(ui::Icon::Folder, "No subfolders",
                       "The scanned folder contains no subfolders, so there is nothing to rank.");
        return;
    }
    if (rows.empty()) {
        ui::EmptyState(ui::Icon::Chart, "Nothing ranked yet",
                       "Rebuild the ranking from Settings or run a new scan.");
        return;
    }

    drawTable(state, rows, searching);
    (void)colors;
}

}  // namespace diskscope
