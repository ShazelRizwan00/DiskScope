#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "core/FolderNode.h"
#include "ui/AppState.h"

namespace diskscope {

class DirectoryExplorer {
public:
    DirectoryExplorer() = default;
    void draw(AppState& state);

private:
    std::uint64_t cachedGeneration_ = 0;
    NodeId selectedFolderId_ = kInvalidNodeId;
    std::vector<const FolderNode*> breadcrumbs_;
    std::string searchFilter_;
};


}  // namespace diskscope

