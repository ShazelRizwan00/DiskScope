#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/FolderNode.h"
#include "structures/HeapEvent.h"
#include "ui/AppState.h"

namespace diskscope {

class HeapVisualizerView {
public:
    HeapVisualizerView() = default;
    void draw(AppState& state);

private:
    std::uint64_t cachedGeneration_ = 0;
    std::vector<HeapEvent> frames_;                  // recorded real heap operations
    std::vector<const FolderNode*> demoItems_;       // <= 63 real folders, level order
    std::size_t frameIndex_ = 0;
    bool playing_ = false;
    bool hasDemo_ = false;
    float accumulator_ = 0.0f;
    int selectedOperation_ = 0;                      // 0 build, 1 insert, 2 extract
    std::string lastAction_;
};

}  // namespace diskscope
