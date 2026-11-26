//Shazel (B)

#pragma once

#include <cstddef>
#include <vector>

#include "core/FolderNode.h"
#include "structures/HeapEvent.h"

namespace diskscope {

// Records the *real* MaxHeap operations on a *copy* of the current items so the
// visualizer can replay them frame by frame.
//
// Nothing here re-implements the heap: every frame comes from MaxHeap itself,
// driven through the IHeapObserver hook, and the live ranking heap is never
// mutated. That is what keeps the animation honest.
class HeapDemoEngine {
public:
    // Default number of nodes handed to the visualizer: 63 keeps the drawing
    // area to six levels, which stays legible and readable in a viva.
    static constexpr std::size_t kDefaultDemoCap = 63;

    // First `cap` items in level order of a valid heap array, i.e. a complete
    // prefix that is still a valid heap.
    //
    // For an insert demo pass cap - 1 so the inserted node cannot push the demo
    // past the node cap and add a seventh level.
    static std::vector<const FolderNode*> selectDemoItems(
        const std::vector<const FolderNode*>& heapItems,
        std::size_t cap = kDefaultDemoCap);

    // Bottom-up construction from an arbitrary item order, with Build /
    // SiftDown / Compare / Swap frames.
    std::vector<HeapEvent> recordBuild(const std::vector<const FolderNode*>& items) const;

    // Insert one folder into a copy of `existingItems` (SiftUp frames).
    std::vector<HeapEvent> recordInsert(const std::vector<const FolderNode*>& existingItems,
                                        const FolderNode* node) const;

    // Extract the maximum from a copy of `existingItems` (Extract / SiftDown
    // frames), including the final Done frame that shows the new root.
    std::vector<HeapEvent> recordExtract(const std::vector<const FolderNode*>& existingItems) const;
};

}  // namespace diskscope
