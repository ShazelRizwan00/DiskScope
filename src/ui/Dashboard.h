#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/FolderNode.h"
#include "ui/AppState.h"

namespace diskscope {

class Dashboard {
public:
    Dashboard() = default;
    void draw(AppState& state);

private:
    std::uint64_t cachedGeneration_ = 0;
    std::vector<const FolderNode*> cachedTopFolders_;
    std::vector<double> chartValues_;
    std::vector<std::string> chartLabels_;
};


}  // namespace diskscope

