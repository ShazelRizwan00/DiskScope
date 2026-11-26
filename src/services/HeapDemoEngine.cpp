//Shazel (B)


#include "services/HeapDemoEngine.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "structures/MaxHeap.h"

namespace diskscope {
namespace {

// Collects every frame the real heap emits. Nothing here re-implements the
// algorithm: the frames are a by-product of MaxHeap's own sift code.
struct Collector : IHeapObserver {
    std::vector<HeapEvent> frames;

    void onHeapEvent(const HeapEvent& event) override { frames.push_back(event); }
};

// The first `cap` slots of a valid heap array form a complete binary tree
// prefix, so they are still a valid heap of real folders.
std::vector<const FolderNode*> firstSlots(const std::vector<const FolderNode*>& items,
                                         std::size_t cap) {
    const std::size_t take = std::min(cap, items.size());
    return std::vector<const FolderNode*>(items.begin(), items.begin() + static_cast<std::ptrdiff_t>(take));
}

}  // namespace

std::vector<const FolderNode*> HeapDemoEngine::selectDemoItems(
    const std::vector<const FolderNode*>& heapItems, std::size_t cap) {
    if (cap == 0) {
        return {};
    }
    return firstSlots(heapItems, cap);
}

std::vector<HeapEvent> HeapDemoEngine::recordBuild(
    const std::vector<const FolderNode*>& items) const {
    Collector collector;
    MaxHeap heap(&collector);
    heap.buildHeap(items);
    return collector.frames;
}

std::vector<HeapEvent> HeapDemoEngine::recordInsert(
    const std::vector<const FolderNode*>& existingItems, const FolderNode* node) const {
    if (node == nullptr) {
        return {};
    }

    Collector collector;
    MaxHeap heap(&collector);

    // Loading the copy emits frames too; they describe work that already
    // happened before the demo, so they are dropped.
    heap.buildHeap(existingItems);
    collector.frames.clear();

    heap.insert(node);
    return collector.frames;
}

std::vector<HeapEvent> HeapDemoEngine::recordExtract(
    const std::vector<const FolderNode*>& existingItems) const {
    if (existingItems.empty()) {
        return {};
    }

    Collector collector;
    MaxHeap heap(&collector);
    heap.buildHeap(existingItems);
    collector.frames.clear();

    heap.extractMax();
    return collector.frames;
}

}  // namespace diskscope
