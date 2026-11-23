#include "ui/LargestFoldersView.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <imgui.h>

#include "core/FolderNode.h"
#include "services/AnalysisEngine.h"
#include "ui/AppState.h"
#include "ui/Theme.h"
#include "utils/Formatting.h"

namespace diskscope {

void LargestFoldersView::draw(AppState& state) {
    if (!state.engine || !state.engine->hasResult()) {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.y > 140.0f) {
            ImGui::Dummy(ImVec2(0.0f, (avail.y - 100.0f) * 0.35f));
        }
        ImGui::PushFont(theme::heading());
        const char* headingText = "Largest Folders";
        const float hWidth = ImGui::CalcTextSize(headingText).x;
        if (avail.x > hWidth) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - hWidth) * 0.5f);
        }
        ImGui::TextUnformatted(headingText);
        ImGui::PopFont();

        ImGui::Spacing();
        const char* subtitle = "No scan loaded. Choose a folder and click Scan to analyze disk space.";
        const float sWidth = ImGui::CalcTextSize(subtitle).x;
        if (avail.x > sWidth) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - sWidth) * 0.5f);
        }
        ImGui::TextColored(theme::colors().textMutedV, "%s", subtitle);
        return;
    }

    const AnalysisStats stats = state.engine->stats();

    const bool needRecompute = (cachedGeneration_ != state.engine->generation())
                            || (cachedK_ != static_cast<std::size_t>(state.topKChoice))
                            || (cachedIncludeRoot_ != state.includeRoot)
                            || (cachedSearchTerm_ != state.searchTerm);
    if (needRecompute) {
        cachedGeneration_ = state.engine->generation();
        cachedK_ = static_cast<std::size_t>(state.topKChoice);
        cachedIncludeRoot_ = state.includeRoot;
        cachedSearchTerm_ = state.searchTerm;
        if (state.searchTerm.empty()) {
            cachedTopK_ = state.engine->topK(cachedK_);
            cachedSearchResults_.clear();
        } else {
            cachedSearchResults_ = state.engine->search(state.searchTerm, 50);
            cachedTopK_.clear();
        }
    }

    // Header section
    ImGui::PushFont(theme::heading());
    ImGui::TextUnformatted("Largest Folders");
    ImGui::PopFont();

    ImGui::TextColored(theme::colors().textMutedV, "%s folders ranked · largest first",
                       format::count(stats.indexedFolders).c_str());

    ImGui::Spacing();

    // Segmented selector for Top 5 / 10 / 20
    const int choices[3] = {5, 10, 20};
    const char* choiceLabels[3] = {"Top 5", "Top 10", "Top 20"};
    for (int i = 0; i < 3; ++i) {
        if (i > 0) {
            ImGui::SameLine(0.0f, 6.0f);
        }
        const bool active = (state.topKChoice == choices[i]);
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, theme::colors().accentSoftV);
            ImGui::PushStyleColor(ImGuiCol_Text, theme::colors().accentV);
            ImGui::PushStyleColor(ImGuiCol_Border, theme::colors().accentV);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, theme::colors().cardV);
            ImGui::PushStyleColor(ImGuiCol_Text, theme::colors().textV);
            ImGui::PushStyleColor(ImGuiCol_Border, theme::colors().borderV);
        }
        if (ImGui::Button(choiceLabels[i], ImVec2(72.0f, 26.0f))) {
            state.topKChoice = choices[i];
        }
        ImGui::PopStyleColor(3);
    }

    // Search box
    ImGui::SameLine(0.0f, 24.0f);
    char searchBuf[256];
    std::strncpy(searchBuf, state.searchTerm.c_str(), sizeof(searchBuf) - 1);
    searchBuf[sizeof(searchBuf) - 1] = '\0';
    ImGui::SetNextItemWidth(260.0f);
    if (ImGui::InputTextWithHint("##FolderSearch", "Filter folders by name...", searchBuf, sizeof(searchBuf))) {
        state.searchTerm = searchBuf;
    }
    ImGui::SameLine(0.0f, 6.0f);
    if (ImGui::Button("Clear") && !state.searchTerm.empty()) {
        state.searchTerm.clear();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const bool isSearching = !state.searchTerm.empty();
    const std::vector<const FolderNode*>& displayList = isSearching ? cachedSearchResults_ : cachedTopK_;

    if (stats.folderCount == 0) {
        ImGui::TextColored(theme::colors().textMutedV, "No subfolders found in the scanned directory.");
        return;
    }

    if (isSearching && displayList.empty()) {
        ImGui::TextColored(theme::colors().textMutedV, "No folders matching \"%s\" were found.",
                           state.searchTerm.c_str());
        return;
    }

    // Table view
    const ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg
                                     | ImGuiTableFlags_BordersInnerV
                                     | ImGuiTableFlags_BordersOuterH
                                     | ImGuiTableFlags_SizingFixedFit
                                     | ImGuiTableFlags_ScrollY
                                     | ImGuiTableFlags_ScrollX;

    if (ImGui::BeginTable("LargestFoldersTable", 5, tableFlags, ImVec2(0.0f, 0.0f))) {
        ImGui::TableSetupColumn("Rank (#)", ImGuiTableColumnFlags_WidthFixed, 65.0f);
        ImGui::TableSetupColumn("Folder", ImGuiTableColumnFlags_WidthStretch, 0.25f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Share", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch, 0.55f);
        ImGui::TableHeadersRow();

        const std::uintmax_t rootTotal = state.engine->result().root ? state.engine->result().root->totalBytes : 0;

        for (std::size_t i = 0; i < displayList.size(); ++i) {
            const FolderNode* node = displayList[i];
            if (!node) {
                continue;
            }

            ImGui::TableNextRow();
            const bool isSelected = (node->id == state.selectedId);

            ImGui::TableSetColumnIndex(0);
            char rankBuf[32];
            std::snprintf(rankBuf, sizeof(rankBuf), "#%zu", i + 1);
            if (ImGui::Selectable(rankBuf, isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0.0f, 0.0f))) {
                state.selectedId = node->id;
                state.requestedSection = 1;  // Jump to directory explorer
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(node->name.c_str());

            ImGui::TableSetColumnIndex(2);
            const std::string sizeStr = format::byteSize(node->totalBytes);
            ImGui::PushFont(theme::mono());
            const float avail = ImGui::GetContentRegionAvail().x;
            const float textW = ImGui::CalcTextSize(sizeStr.c_str()).x;
            if (avail > textW) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - textW);
            }
            ImGui::TextUnformatted(sizeStr.c_str());
            ImGui::PopFont();

            ImGui::TableSetColumnIndex(3);
            const double share = (rootTotal > 0) ? (static_cast<double>(node->totalBytes) / static_cast<double>(rootTotal)) : 0.0;
            ImGui::TextUnformatted(format::percent(share).c_str());

            ImGui::TableSetColumnIndex(4);
            const std::string fullPath = node->path.string();
            const std::string ellipPath = format::ellipsize(fullPath, 65);
            ImGui::TextUnformatted(ellipPath.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", fullPath.c_str());
            }
        }

        ImGui::EndTable();
    }
}

}  // namespace diskscope
