#include "ui/DirectoryExplorer.h"

#include <algorithm>
#include <cmath>
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

constexpr float kRowHeight = 26.0f;
constexpr float kIndentStep = 14.0f;
constexpr float kSplitLeftRatio = 0.42f;
constexpr float kSplitLeftMin = 280.0f;

ImU32 withAlpha(ImU32 colour, float alpha) {
    ImVec4 value = ImGui::ColorConvertU32ToFloat4(colour);
    value.w *= alpha;
    return ImGui::ColorConvertFloat4ToU32(value);
}

// Column widths are derived from the measured text rather than assumed, because
// the mono font is wider than the body font at the same nominal size.
void rightAlignedText(float rightX, ImFont* font, ImU32 colour, const char* text) {
    const ImVec2 size = font->CalcTextSizeA(font->FontSize, 1000.0f, 0.0f, text);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddText(font, font->FontSize, ImVec2(rightX - size.x, p.y), colour, text);
}

void headerCellRight(const char* text) {
    ImFont* font = theme::body();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float right = start.x + ImGui::GetContentRegionAvail().x;
    const ImVec2 size = font->CalcTextSizeA(font->FontSize, 1000.0f, 0.0f, text);
    ImGui::GetWindowDrawList()->AddText(font, font->FontSize, ImVec2(right - size.x, start.y),
                                        theme::colors().textSecondary, text);
    ImGui::Dummy(ImVec2(0.0f, size.y));
}

}  // namespace

void DirectoryExplorer::expandAllBounded(const FolderNode* node, int depth) {
    if (node == nullptr || depth > 24) {
        return;
    }
    for (const auto& child : node->children) {
        if (expandCounter_ >= kExpandBudget) {
            return;
        }
        expanded_.insert(child->id);
        ++expandCounter_;
        expandAllBounded(child.get(), depth + 1);
    }
}

void DirectoryExplorer::refreshFilter(AppState& state) {
    if (!state.engine || !state.engine->hasResult()) {
        filterResults_.clear();
        filterCache_.clear();
        return;
    }
    if (filterCache_ == state.explorerFilter && cachedGeneration_ == state.engine->generation()) {
        return;
    }
    filterCache_ = state.explorerFilter;
    filterResults_.clear();
    if (!state.explorerFilter.empty()) {
        filterResults_ = state.engine->search(state.explorerFilter, 200);
    }
}

float DirectoryExplorer::drawTreeRow(const FolderNode* node, int depth, AppState& state, float width,
                                    float y) {
    const theme::Colors& colors = theme::colors();
    const bool hasChildren = !node->leaf();
    const bool expanded = expanded_.find(node->id) != expanded_.end();
    const bool selected = (state.selectedId == node->id);

    const float indent = static_cast<float>(depth) * kIndentStep;
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, y));
    const ImVec2 rowMin = ImGui::GetCursorScreenPos();
    const ImVec2 rowMax(rowMin.x + width, rowMin.y + kRowHeight);
    const float centreY = rowMin.y + kRowHeight * 0.5f;
    const float chevronX = rowMin.x + indent + 8.0f;

    ImGui::PushID(static_cast<int>(node->id));
    ImGui::InvisibleButton("##row", ImVec2(width, kRowHeight));
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked(0);
    const bool doubleClicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0);

    if (clicked) {
        const float mouseX = ImGui::GetIO().MousePos.x;
        const bool onChevron = hasChildren && std::fabs(mouseX - chevronX) <= 10.0f;
        state.selectedId = node->id;
        if (onChevron) {
            if (expanded) {
                expanded_.erase(node->id);
            } else {
                expanded_.insert(node->id);
            }
        }
    } else if (doubleClicked && hasChildren) {
        if (expanded) {
            expanded_.erase(node->id);
        } else {
            expanded_.insert(node->id);
        }
    }

    ImDrawList* list = ImGui::GetWindowDrawList();
    if (selected) {
        list->AddRectFilled(rowMin, rowMax, colors.greenSoft, 5.0f);
        list->AddRectFilled(ImVec2(rowMin.x, rowMin.y + 4.0f), ImVec2(rowMin.x + 2.0f, rowMax.y - 4.0f),
                            colors.green, 1.0f);
    } else if (hovered) {
        list->AddRectFilled(rowMin, rowMax, colors.elevatedHover, 5.0f);
    }

    if (hasChildren) {
        ui::DrawIcon(list, expanded ? ui::Icon::ChevronDown : ui::Icon::ChevronRight,
                     ImVec2(chevronX, centreY), 12.0f,
                     selected ? colors.green : colors.textSecondary, 1.5f);
    }

    ui::DrawIcon(list, expanded ? ui::Icon::FolderOpen : ui::Icon::Folder,
                 ImVec2(rowMin.x + indent + 24.0f, centreY), 16.0f,
                 selected ? colors.green : colors.textSecondary, 1.5f);

    const std::uintmax_t rootTotal =
        state.engine->result().root ? state.engine->result().root->totalBytes : 0;
    const double share =
        (rootTotal > 0) ? static_cast<double>(node->totalBytes) / static_cast<double>(rootTotal) : 0.0;

    ImFont* monoFont = theme::mono();
    ImFont* captionFont = theme::caption();
    const std::string sizeText = format::byteSize(node->totalBytes);
    const std::string shareText = format::percent(share);
    const ImVec2 sizeSize = monoFont->CalcTextSizeA(monoFont->FontSize, 1000.0f, 0.0f, sizeText.c_str());
    const ImVec2 shareSize = captionFont->CalcTextSizeA(captionFont->FontSize, 1000.0f, 0.0f, shareText.c_str());

    const float sizeRight = rowMax.x - 8.0f;
    const float sizeLeft = sizeRight - sizeSize.x;
    const float shareRight = sizeLeft - 10.0f;
    const float shareLeft = shareRight - shareSize.x;
    const float nameX = rowMin.x + indent + 40.0f;
    const float nameRoom = (std::max)(20.0f, shareLeft - 8.0f - nameX);
    const std::size_t maxChars = static_cast<std::size_t>((std::max)(6.0f, nameRoom / 6.6f));

    ImGui::PushFont(theme::body());
    list->AddText(theme::body(), theme::body()->FontSize,
                  ImVec2(nameX, centreY - theme::body()->FontSize * 0.5f),
                  selected ? colors.green : colors.text,
                  format::ellipsize(node->name, maxChars).c_str());
    ImGui::PopFont();

    list->AddText(captionFont, captionFont->FontSize, ImVec2(shareLeft, centreY - shareSize.y * 0.5f),
                  colors.textSecondary, shareText.c_str());
    list->AddText(monoFont, monoFont->FontSize, ImVec2(sizeLeft, centreY - sizeSize.y * 0.5f),
                  colors.textSecondary, sizeText.c_str());

    // Share meter along the bottom edge: a quiet way to compare siblings without
    // adding another column.
    if (share > 0.0) {
        const float meterW = static_cast<float>(std::min(1.0, share)) * width;
        list->AddRectFilled(ImVec2(rowMin.x + 1.0f, rowMax.y - 2.0f),
                            ImVec2(rowMin.x + 1.0f + meterW, rowMax.y - 0.5f),
                            withAlpha(colors.green, 0.55f));
    }

    ImGui::PopID();

    float nextY = rowMax.y;
    if (expanded) {
        for (const auto& child : node->children) {
            nextY = drawTreeRow(child.get(), depth + 1, state, width, nextY);
        }
    }
    return nextY;
}

void DirectoryExplorer::drawFilterResults(AppState& state, float width) {
    const theme::Colors& colors = theme::colors();
    refreshFilter(state);

    if (filterResults_.empty()) {
        ui::EmptyState(ui::Icon::Search, "No matches",
                       "No folder name in this scan contains the filter text.");
        return;
    }

    ImGui::PushFont(theme::caption());
    ImGui::TextColored(colors.textSecondaryV, "%s folders match",
                       format::count(filterResults_.size()).c_str());
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    for (const FolderNode* node : filterResults_) {
        if (node == nullptr) {
            continue;
        }
        ImGui::PushID(static_cast<int>(node->id));
        const ImVec2 rowStart = ImGui::GetCursorScreenPos();
        const bool selected = (state.selectedId == node->id);
        ImGui::InvisibleButton("##match", ImVec2(width, 34.0f));
        const bool hovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked(0)) {
            state.selectedId = node->id;
        }

        ImDrawList* list = ImGui::GetWindowDrawList();
        const ImVec2 rowMax(rowStart.x + width, rowStart.y + 34.0f);
        if (selected) {
            list->AddRectFilled(rowStart, rowMax, colors.greenSoft, 5.0f);
        } else if (hovered) {
            list->AddRectFilled(rowStart, rowMax, colors.elevatedHover, 5.0f);
        }

        ui::DrawIcon(list, ui::Icon::Folder, ImVec2(rowStart.x + 10.0f, rowStart.y + 12.0f), 14.0f,
                     selected ? colors.green : colors.textSecondary, 1.5f);
        list->AddText(theme::body(), theme::body()->FontSize,
                      ImVec2(rowStart.x + 26.0f, rowStart.y + 3.0f),
                      selected ? colors.green : colors.text, format::ellipsize(node->name, 42).c_str());

        ImFont* captionFont = theme::caption();
        const std::string pathText = format::ellipsize(node->path.string(), 70);
        list->AddText(captionFont, captionFont->FontSize, ImVec2(rowStart.x + 26.0f, rowStart.y + 19.0f),
                      colors.textSecondary, pathText.c_str());

        const std::string sizeText = format::byteSize(node->totalBytes);
        ImFont* monoFont = theme::mono();
        const ImVec2 sizeSize = monoFont->CalcTextSizeA(monoFont->FontSize, 1000.0f, 0.0f, sizeText.c_str());
        list->AddText(monoFont, monoFont->FontSize,
                      ImVec2(rowMax.x - 8.0f - sizeSize.x, rowStart.y + 6.0f), colors.textSecondary,
                      sizeText.c_str());

        ImGui::PopID();
    }
}

void DirectoryExplorer::drawTreeCard(AppState& state, float height) {
    const theme::Colors& colors = theme::colors();
    const FolderNode* root = state.engine->result().root.get();
    const std::string subtitle =
        root ? format::count(root->folderCount) + " subfolders in this scan" : std::string();

    const ui::CardOptions options{"Folder Tree", subtitle.c_str(), ui::Icon::FolderTree, height};
    if (ui::BeginCard("FolderTreeCard", options)) {
        const float available = ImGui::GetContentRegionAvail().x;
        const bool treeMode = state.explorerFilter.empty();
        const float controlW = treeMode ? 148.0f : 0.0f;
        const float searchW = (std::max)(140.0f, available - controlW - 8.0f);

        ui::SearchInput("##explorerFilter", state.explorerFilter, "Filter folders by name", searchW);
        if (treeMode) {
            ImGui::SameLine(0.0f, 8.0f);
            if (ui::Button("##expandAll", "Expand", ui::ButtonRole::Ghost, ui::ButtonSize::Small,
                           ui::Icon::ChevronDown, 70.0f)) {
                expandCounter_ = 0;
                expanded_.insert(root ? root->id : kInvalidNodeId);
                expandAllBounded(root, 0);
            }
            ImGui::SameLine(0.0f, 4.0f);
            if (ui::Button("##collapseAll", "Collapse", ui::ButtonRole::Ghost, ui::ButtonSize::Small,
                           ui::Icon::ChevronRight, 70.0f)) {
                expanded_.clear();
            }
        }

        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        ui::Divider(false);
        ImGui::Dummy(ImVec2(0.0f, 2.0f));

        const float contentW = ImGui::GetContentRegionAvail().x;
        if (!treeMode) {
            drawFilterResults(state, contentW);
        } else if (root == nullptr) {
            ui::EmptyState(ui::Icon::Folder, "Nothing scanned",
                           "Choose a folder in the toolbar and press Scan.");
        } else {
            float y = ImGui::GetCursorScreenPos().y;
            y = drawTreeRow(root, 0, state, contentW, y);
            ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, y));

            if (root->children.empty()) {
                ImGui::PushFont(theme::caption());
                ImGui::TextColored(colors.textSecondaryV, "This folder has no subfolders.");
                ImGui::PopFont();
            }
        }
    }
    ui::EndCard();
}

void DirectoryExplorer::drawMetricTiles(const FolderNode* node, std::uintmax_t rootTotal, float width) {
    const theme::Colors& colors = theme::colors();
    const float gap = theme::metrics().gap;
    const float tileW = (width - 2.0f * gap) / 3.0f;
    const float tileH = 62.0f;

    const double share = (rootTotal > 0) ? static_cast<double>(node->totalBytes) /
                                               static_cast<double>(rootTotal)
                                         : 0.0;

    struct Tile {
        const char* label;
        std::string value;
        ui::Tone tone;
    };
    const Tile tiles[3] = {
        {"Inclusive size", format::byteSize(node->totalBytes), ui::Tone::Green},
        {"On disk", format::byteSize(node->allocatedBytes), ui::Tone::Neutral},
        {"Share of root", format::percent(share), ui::Tone::Neutral},
    };

    for (int i = 0; i < 3; ++i) {
        if (i > 0) {
            ImGui::SameLine(0.0f, gap);
        }
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImVec2 max(origin.x + tileW, origin.y + tileH);
        ImDrawList* list = ImGui::GetWindowDrawList();
        list->AddRectFilled(origin, max, colors.elevated, 8.0f);
        list->AddRect(origin, max, colors.border, 8.0f, 0, 1.0f);

        ImGui::PushFont(theme::caption());
        list->AddText(theme::caption(), theme::caption()->FontSize, ImVec2(origin.x + 10.0f, origin.y + 8.0f),
                      colors.textSecondary, tiles[i].label);
        ImGui::PopFont();

        const ImU32 valueColour = (tiles[i].tone == ui::Tone::Green) ? colors.green : colors.text;
        ImFont* valueFont = theme::h3();
        list->AddText(valueFont, valueFont->FontSize, ImVec2(origin.x + 10.0f, origin.y + 28.0f), valueColour,
                      tiles[i].value.c_str());

        ImGui::Dummy(ImVec2(tileW, tileH));
    }
}

void DirectoryExplorer::drawChildren(const FolderNode* node, AppState& state) {
    const theme::Colors& colors = theme::colors();

    if (childrenOwner_ != node->id || cachedGeneration_ == 0) {
        childrenOwner_ = node->id;
        sortedChildren_.clear();
        sortedChildren_.reserve(node->children.size());
        for (const auto& child : node->children) {
            sortedChildren_.push_back(child.get());
        }
        std::sort(sortedChildren_.begin(), sortedChildren_.end(),
                  [](const FolderNode* lhs, const FolderNode* rhs) {
                      if (lhs->totalBytes != rhs->totalBytes) {
                          return lhs->totalBytes > rhs->totalBytes;
                      }
                      return lhs->name < rhs->name;
                  });
    }

    ImGui::PushFont(theme::bodyStrong());
    ImGui::TextColored(colors.textV, "Subfolders");
    ImGui::PopFont();
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::PushFont(theme::caption());
    ImGui::TextColored(colors.textSecondaryV, "%s", format::count(node->children.size()).c_str());
    ImGui::PopFont();

    if (sortedChildren_.empty()) {
        ImGui::PushFont(theme::caption());
        ImGui::TextColored(colors.textSecondaryV,
                           "No subfolders here. The files in this folder are counted directly.");
        ImGui::PopFont();
        return;
    }

    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp |
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerH;
    if (ImGui::BeginTable("##childTable", 3, flags, ImVec2(0.0f, ImGui::GetContentRegionAvail().y))) {
        ImGui::TableSetupColumn("Folder", ImGuiTableColumnFlags_WidthStretch, 0.55f);
        ImGui::TableSetupColumn("Usage", ImGuiTableColumnFlags_WidthStretch, 0.28f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 96.0f);
        ImGui::TableSetupScrollFreeze(0, 1);

        // Manual header row: the Size label must sit over right-aligned values.
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        ImGui::TableSetColumnIndex(0);
        ImGui::TextColored(colors.textSecondaryV, "Folder");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(colors.textSecondaryV, "Usage");
        ImGui::TableSetColumnIndex(2);
        headerCellRight("Size");

        const std::uintmax_t parentTotal = (node->totalBytes > 0) ? node->totalBytes : 1;
        for (const FolderNode* child : sortedChildren_) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushID(static_cast<int>(child->id));
            if (ImGui::Selectable(child->name.c_str(), state.selectedId == child->id,
                                  ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, 22.0f))) {
                state.selectedId = child->id;
            }
            if (ImGui::IsItemHovered()) {
                ui::Tooltip(child->path.string().c_str());
            }
            ImGui::PopID();

            ImGui::TableSetColumnIndex(1);
            const float share = static_cast<float>(static_cast<double>(child->totalBytes) /
                                                   static_cast<double>(parentTotal));
            ui::BarMeter(share, ImGui::GetContentRegionAvail().x, ui::Tone::Green, 5.0f);

            ImGui::TableSetColumnIndex(2);
            rightAlignedText(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x,
                             theme::mono(), colors.textSecondary, format::byteSize(child->totalBytes).c_str());
        }
        ImGui::EndTable();
    }
}

void DirectoryExplorer::drawDetailsCard(AppState& state, float height) {
    const theme::Colors& colors = theme::colors();
    const FolderNode* root = state.engine->result().root.get();
    const FolderNode* node = (state.selectedId != kInvalidNodeId) ? state.engine->find(state.selectedId)
                                                                  : nullptr;

    const ui::CardOptions options{"Folder Details", nullptr, ui::Icon::Folder, height};
    if (ui::BeginCard("FolderDetailsCard", options)) {
        if (node == nullptr) {
            ui::EmptyState(ui::Icon::FolderOpen, "No folder selected",
                           "Pick a folder from the tree to inspect what it holds.");
            ui::EndCard();
            return;
        }

        const float width = ImGui::GetContentRegionAvail().x;

        ImGui::PushFont(theme::h3());
        ImGui::TextColored(colors.textV, "%s", node->name.c_str());
        ImGui::PopFont();
        ImGui::PushFont(theme::caption());
        ImGui::TextColored(colors.textSecondaryV, "%s",
                           format::ellipsize(node->path.string(), 96).c_str());
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0.0f, 6.0f));

        // Breadcrumb: every ancestor is a jump target, so the path doubles as
        // navigation instead of being a dead label. Deep trees collapse the middle
        // so the trail never overflows the pane.
        const std::vector<const FolderNode*> trail = state.engine->pathTo(node);
        std::vector<const FolderNode*> crumbs = trail;
        bool elided = false;
        if (crumbs.size() > 4) {
            crumbs = {trail.front(), trail[trail.size() - 2], trail.back()};
            elided = true;
        }
        for (std::size_t i = 0; i < crumbs.size(); ++i) {
            if (i > 0) {
                ImGui::SameLine(0.0f, 4.0f);
                ImGui::PushFont(theme::caption());
                ImGui::TextColored(colors.textSecondaryV, "%s", (elided && i == 1) ? "... >" : ">");
                ImGui::PopFont();
                ImGui::SameLine(0.0f, 4.0f);
            }
            ImGui::PushID(static_cast<int>(i));
            const std::string label = format::ellipsize(crumbs[i]->name, 16);
            if (ui::Button("##crumb", label.c_str(), ui::ButtonRole::Ghost, ui::ButtonSize::Small,
                           ui::Icon::None, 0.0f)) {
                state.selectedId = crumbs[i]->id;
            }
            ImGui::PopID();
        }

        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        drawMetricTiles(node, root ? root->totalBytes : 0, width);

        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        const std::uintmax_t rootTotal = root ? root->totalBytes : 0;
        const float share = (rootTotal > 0) ? static_cast<float>(static_cast<double>(node->totalBytes) /
                                                                 static_cast<double>(rootTotal))
                                            : 0.0f;
        ui::BarMeter(share, width, ui::Tone::Green, 6.0f);
        ImGui::PushFont(theme::caption());
        ImGui::TextColored(colors.textSecondaryV, "%s of the scanned total",
                           format::percent(share).c_str());
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0.0f, 8.0f));

        ui::KeyValueRow("Files in subtree", format::count(node->fileCount).c_str(), 150.0f);
        ui::KeyValueRow("Folders in subtree", format::count(node->folderCount).c_str(), 150.0f);
        const std::string ownFiles = format::count(node->ownFileCount) + "  (" +
                                     format::byteSize(node->ownFileBytes) + ")";
        ui::KeyValueRow("Files directly here", ownFiles.c_str(), 150.0f);

        // Long values are budgeted to the pane width: the details column narrows
        // sharply at the minimum window size and must not run under the card edge.
        const std::size_t valueChars =
            static_cast<std::size_t>((std::max)(18.0f, (width - 165.0f) / 7.0f));
        ui::KeyValueRow("Path", format::ellipsize(node->path.string(), valueChars).c_str(), 150.0f);
        if (node != root && node->path.has_parent_path()) {
            ui::KeyValueRow("Parent",
                            format::ellipsize(node->path.parent_path().string(), valueChars).c_str(),
                            150.0f);
        }

        if (!node->accessible) {
            ImGui::Dummy(ImVec2(0.0f, 4.0f));
            const ImVec2 start = ImGui::GetCursorScreenPos();
            ui::DrawIcon(ImGui::GetWindowDrawList(), ui::Icon::AlertTriangle,
                         ImVec2(start.x + 8.0f, start.y + ImGui::GetTextLineHeight() * 0.5f), 15.0f,
                         colors.red, 1.5f);
            ImGui::Dummy(ImVec2(20.0f, ImGui::GetTextLineHeight()));
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::TextColored(colors.redV, "Some entries in this folder could not be read.");
        }

        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        drawChildren(node, state);
    }
    ui::EndCard();
}

void DirectoryExplorer::draw(AppState& state) {
    ui::SectionHeader(ui::Icon::Folder, "Directory Explorer",
                      "Drill into the scanned hierarchy and inspect one folder at a time");

    if (!state.engine || !state.engine->hasResult()) {
        ui::EmptyState(ui::Icon::FolderOpen, "No scan loaded",
                       "Choose a folder in the toolbar and press Scan to explore its hierarchy.");
        return;
    }

    if (cachedGeneration_ != state.engine->generation()) {
        cachedGeneration_ = state.engine->generation();
        expanded_.clear();
        filterResults_.clear();
        filterCache_.clear();
        childrenOwner_ = kInvalidNodeId;
        sortedChildren_.clear();
        if (state.engine->result().root) {
            expanded_.insert(state.engine->result().root->id);
        }
    }

    const float width = ImGui::GetContentRegionAvail().x;
    const float height = ImGui::GetContentRegionAvail().y;
    // A fixed split keeps the tree readable while the details pane takes the rest.
    const float leftW = (std::max)(kSplitLeftMin, width * kSplitLeftRatio);
    const float rightW = (std::max)(240.0f, width - leftW - theme::metrics().gap);

    // The cards always fill their column, so the table owns the split geometry.
    if (ImGui::BeginTable("##explorerSplit", 2, ImGuiTableFlags_SizingFixedFit |
                                                 ImGuiTableFlags_NoPadOuterX |
                                                 ImGuiTableFlags_NoPadInnerX)) {
        ImGui::TableSetupColumn("##treeP", ImGuiTableColumnFlags_WidthFixed, leftW);
        ImGui::TableSetupColumn("##detailP", ImGuiTableColumnFlags_WidthFixed, rightW);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        drawTreeCard(state, height);
        ImGui::TableNextColumn();
        drawDetailsCard(state, height);
        ImGui::EndTable();
    }
}

}  // namespace diskscope
