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
    std::vector<HeapEvent> frames_;
    std::size_t frameIndex_ = 0;
    bool playing_ = false;
    float accumulator_ = 0.0f;
    int selectedOperation_ = 0;
};


}  // namespace diskscope

