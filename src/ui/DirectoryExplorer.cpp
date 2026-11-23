#include "ui/DirectoryExplorer.h"

#include <cstdint>
#include <string>
#include <vector>

#include <imgui.h>

#include "core/FolderNode.h"
#include "services/AnalysisEngine.h"
#include "ui/AppState.h"
#include "ui/Theme.h"
#include "utils/Formatting.h"

namespace diskscope {

void DirectoryExplorer::drawNode(const FolderNode* node, AppState& state) {
    if (!node) {
        return;
    }

    for (NodeId id : nodesToOpen_) {
        if (id == node->id) {
            ImGui::SetNextItemOpen(true);
            break;
        }
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                             | ImGuiTreeNodeFlags_OpenOnDoubleClick
                             | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (node->leaf()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (node->id == state.selectedId) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    const void* nodeIdPtr = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(node->id));
    const bool open = ImGui::TreeNodeEx(nodeIdPtr, flags, "%s", node->name.c_str());

    if (ImGui::IsItemClicked()) {
        state.selectedId = node->id;
    }

    const std::string sizeStr = format::byteSize(node->totalBytes);
    const float textW = ImGui::CalcTextSize(sizeStr.c_str()).x;
    const float rightX = ImGui::GetWindowContentRegionMax().x - textW - 8.0f;
    if (rightX > ImGui::GetCursorPosX() + 8.0f) {
        ImGui::SameLine(rightX);
        ImGui::TextColored(theme::colors().textMutedV, "%s", sizeStr.c_str());
    }

    if (open) {
        ImGui::TextColored(theme::colors().textMutedV, "  %s %s",
                           format::count(node->folderCount).c_str(),
                           node->folderCount == 1 ? "folder" : "folders");

        for (const auto& child : node->children) {
            drawNode(child.get(), state);
        }
        ImGui::TreePop();
    }
}

void DirectoryExplorer::draw(AppState& state) {
    if (!state.engine || !state.engine->hasResult()) {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.y > 140.0f) {
            ImGui::Dummy(ImVec2(0.0f, (avail.y - 100.0f) * 0.35f));
        }
        ImGui::PushFont(theme::heading());
        const char* headingText = "Directory Explorer";
        const float hWidth = ImGui::CalcTextSize(headingText).x;
        if (avail.x > hWidth) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - hWidth) * 0.5f);
        }
        ImGui::TextUnformatted(headingText);
        ImGui::PopFont();

        ImGui::Spacing();
        const char* subtitle = "No scan loaded. Choose a folder above and press Scan to explore directories.";
        const float sWidth = ImGui::CalcTextSize(subtitle).x;
        if (avail.x > sWidth) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - sWidth) * 0.5f);
        }
        ImGui::TextColored(theme::colors().textMutedV, "%s", subtitle);
        return;
    }

    if (cachedGeneration_ != state.engine->generation()) {
        cachedGeneration_ = state.engine->generation();
        selectedFolderId_ = kInvalidNodeId;
        breadcrumbs_.clear();
        nodesToOpen_.clear();
    }

    if (state.selectedId != selectedFolderId_) {
        selectedFolderId_ = state.selectedId;
        breadcrumbs_.clear();
        nodesToOpen_.clear();
        if (selectedFolderId_ != kInvalidNodeId) {
            const FolderNode* target = state.engine->find(selectedFolderId_);
            if (target) {
                breadcrumbs_ = state.engine->pathTo(target);
                for (const FolderNode* p : breadcrumbs_) {
                    if (p && p->id != selectedFolderId_) {
                        nodesToOpen_.push_back(p->id);
                    }
                }
            }
        }
    }

    const float totalW = ImGui::GetContentRegionAvail().x;
    const float leftW = (totalW - 12.0f) * 0.46f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::colors().cardV);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::colors().borderV);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    // Left column (~46% width): Tree view
    if (ImGui::BeginChild("TreePane", ImVec2(leftW, 0.0f), ImGuiChildFlags_Borders)) {
        ImGui::PushFont(theme::heading());
        ImGui::TextUnformatted("Folder Tree");
        ImGui::PopFont();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const FolderNode* root = state.engine->result().root.get();
        if (root) {
            if (root->children.empty()) {
                ImGui::TextColored(theme::colors().textMutedV, "No subdirectories in scan root.");
            } else {
                for (const auto& child : root->children) {
                    drawNode(child.get(), state);
                }
            }
        }
        nodesToOpen_.clear();
    }
    ImGui::EndChild();

    ImGui::SameLine(0.0f, 12.0f);

    // Right column: Detail card for selected node
    if (ImGui::BeginChild("DetailPane", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders)) {
        const FolderNode* selectedNode = (state.selectedId != kInvalidNodeId) ? state.engine->find(state.selectedId) : nullptr;

        if (!selectedNode) {
            ImGui::PushFont(theme::heading());
            ImGui::TextUnformatted("Folder Details");
            ImGui::PopFont();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(theme::colors().textMutedV, "Select a folder from the tree on the left to view its storage details.");
        } else {
            ImGui::PushFont(theme::heading());
            ImGui::TextColored(theme::colors().accentV, "%s", selectedNode->name.c_str());
            ImGui::PopFont();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(theme::colors().textMutedV, "Full path");
            ImGui::TextWrapped("%s", selectedNode->path.string().c_str());
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            const std::uintmax_t rootTotal = state.engine->result().root ? state.engine->result().root->totalBytes : 0;
            const double shareFrac = (rootTotal > 0) ? (static_cast<double>(selectedNode->totalBytes) / static_cast<double>(rootTotal)) : 0.0;

            auto drawField = [](const char* label, const std::string& value) {
                ImGui::TextColored(theme::colors().textMutedV, "%-22s", label);
                ImGui::SameLine(180.0f);
                ImGui::TextUnformatted(value.c_str());
            };

            drawField("Inclusive size", format::byteSize(selectedNode->totalBytes));
            drawField("Allocated on disk", format::byteSize(selectedNode->allocatedBytes));
            drawField("Share of scan root", format::percent(shareFrac));
            drawField("Files in subtree", format::count(selectedNode->fileCount));
            drawField("Folders in subtree", format::count(selectedNode->folderCount));
            drawField("Direct files", format::count(selectedNode->ownFileCount) + " (" + format::byteSize(selectedNode->ownFileBytes) + ")");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            std::string parentPath = "(none — scan root)";
            const FolderNode* root = state.engine->result().root.get();
            if (root && selectedNode->id != root->id) {
                if (selectedNode->path.has_parent_path()) {
                    parentPath = selectedNode->path.parent_path().string();
                }
            }
            ImGui::TextColored(theme::colors().textMutedV, "Parent folder");
            ImGui::TextWrapped("%s", parentPath.c_str());

            if (!selectedNode->accessible) {
                ImGui::Spacing();
                ImGui::TextColored(theme::colors().warnV, "Warning: Some entries inside this folder could not be read.");
            }
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

}  // namespace diskscope
