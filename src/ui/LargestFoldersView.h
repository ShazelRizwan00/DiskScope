#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/FolderNode.h"
#include "ui/AppState.h"

namespace diskscope {

// Ranked list of the largest folders, served straight from the max heap through
// AnalysisEngine::topK. The table is a view over cached results: ranking is an
// O(D + k log D) operation and must not run once per frame.
class LargestFoldersView {
public:
    LargestFoldersView() = default;
    void draw(AppState& state);

private:
    void drawControls(AppState& state, const std::string& summary);
    void drawTable(AppState& state, const std::vector<const FolderNode*>& rows, bool searching);

    std::uint64_t cachedGeneration_ = 0;
    std::size_t cachedK_ = 0;
    bool cachedIncludeRoot_ = false;
    std::string cachedSearchTerm_;
    std::vector<const FolderNode*> cachedTopK_;
    std::vector<const FolderNode*> cachedSearchResults_;
};

}  // namespace diskscope
