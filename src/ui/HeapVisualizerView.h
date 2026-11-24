#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/FolderNode.h"
#include "structures/HeapEvent.h"
#include "ui/AppState.h"

namespace diskscope {

namespace viz {
struct HeapDrawState;
}

// Replays recorded frames of the real MaxHeap: the tree, the level-order array
// and the operation legend all read from the same HeapEvent snapshot, so what is
// drawn can never drift from what the algorithm did.
class HeapVisualizerView {
public:
    HeapVisualizerView() = default;
    void draw(AppState& state);

    // Offline-capture hook: queues a demo and, when requested, pauses on the first
    // recorded operation frame so a headless run captures a mid-playback state
    // instead of the static heap. Later steps must preserve this behaviour.
    void requestDemo(int demo, bool pauseAtFirstHighlight = true);

private:
    void drawControls(AppState& state);
    void runPendingDemo(const std::vector<const FolderNode*>& liveItems);
    void drawCard(AppState& state, const std::vector<const FolderNode*>& liveItems);
    void drawStatusLine(const std::vector<const FolderNode*>& array) const;
    void drawDetails(const FolderNode* node, std::size_t slot, AppState& state, float width);
    void drawArrayStrip(const std::vector<const FolderNode*>& array, std::size_t selectedSlot, float width);
    void drawLegend() const;

    // The frame currently on screen, or nullptr when the demo has recorded nothing.
    const HeapEvent* currentFrame() const;

    // Draw state for the frame on screen, shared by the tree and the array strip so
    // both always highlight the same slots.
    viz::HeapDrawState makeDrawState(std::size_t selectedSlot, std::size_t hoveredSlot) const;

    std::uint64_t cachedGeneration_ = 0;
    std::vector<HeapEvent> frames_;             // recorded real heap operations
    std::vector<const FolderNode*> demoItems_;  // demo subset, level order
    std::size_t frameIndex_ = 0;
    bool playing_ = false;
    float accumulator_ = 0.0f;
    int pendingDemo_ = -1;
    bool stopAtHighlight_ = false;
    std::size_t hoveredSlot_ = kNoIndex;
    float animT_ = 1.0f;       // settle progress of the frame on screen
    float arrayScroll_ = 0.0f; // manual horizontal offset of the array strip
    std::string lastAction_;
};

}  // namespace diskscope
