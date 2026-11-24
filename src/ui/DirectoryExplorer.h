#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

#include "core/FolderNode.h"
#include "ui/AppState.h"

namespace diskscope {

// Two-pane directory browser: the N-ary tree on the left, the details of the
// selected folder on the right.
//
// The scanner keeps no per-file records (only ownFileCount/ownFileBytes per
// directory), so this view shows folders plus an honest aggregate of the files
// sitting directly in each one. It never invents file rows.
class DirectoryExplorer {
public:
    DirectoryExplorer() = default;
    void draw(AppState& state);

private:
    float drawTreeRow(const FolderNode* node, int depth, AppState& state, float width, float y);
    void expandAllBounded(const FolderNode* node, int depth);
    void refreshFilter(AppState& state);
    void drawTreeCard(AppState& state, float height);
    void drawFilterResults(AppState& state, float width);
    void drawDetailsCard(AppState& state, float height);
    void drawMetricTiles(const FolderNode* node, std::uintmax_t rootTotal, float width);
    void drawChildren(const FolderNode* node, AppState& state);

    std::uint64_t cachedGeneration_ = 0;
    std::unordered_set<NodeId> expanded_;

    std::string filterCache_;
    std::vector<const FolderNode*> filterResults_;

    // Children of the selected folder, largest first, rebuilt only when the
    // selection or the tree changes.
    NodeId childrenOwner_ = kInvalidNodeId;
    std::vector<const FolderNode*> sortedChildren_;

    // Bounds an "expand all" on a deep real-world tree: a full expansion of a
    // 100k-folder volume would draw nothing usable anyway.
    static constexpr int kExpandBudget = 1200;
    int expandCounter_ = 0;
};

}  // namespace diskscope
