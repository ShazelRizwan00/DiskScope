#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/FolderNode.h"
#include "ui/AppState.h"

namespace diskscope {

class LargestFoldersView {
public:
    LargestFoldersView() = default;
    void draw(AppState& state);

private:
    std::uint64_t cachedGeneration_ = 0;
    std::size_t cachedK_ = 0;
    bool cachedIncludeRoot_ = false;
    std::string cachedSearchTerm_;
    std::vector<const FolderNode*> cachedTopK_;
    std::vector<const FolderNode*> cachedSearchResults_;
};


}  // namespace diskscope

