#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/FolderNode.h"

namespace diskscope {

// Value used for "no array index" in heap events.
inline constexpr std::size_t kNoIndex = static_cast<std::size_t>(-1);

// What the heap was doing when the event was recorded. The visualizer renders
// one frame per event, in order.
enum class HeapEventKind {
    Build,     // bottom-up heap construction
    Compare,   // two slots were compared (a vs b)
    Swap,      // two slots were exchanged
    SiftUp,    // movement from a child slot up toward the root
    SiftDown,  // movement from a slot down toward the leaves
    Insert,    // a node was appended at slot a
    Extract,   // the root was removed, slot a now holds the last item
    Done,      // operation finished
};

// One recorded step of a real MaxHeap operation.
//
// `array` is the complete level-order backing array *after* the step, so the
// visualizer can draw the tree and the array strip without touching the live
// heap. Snapshots are what make it impossible to fake the visualization: the
// frames come from the actual sift-up / sift-down code.
struct HeapEvent {
    HeapEventKind kind = HeapEventKind::Done;
    std::size_t a = kNoIndex;
    std::size_t b = kNoIndex;
    std::vector<const FolderNode*> array;
    std::string note; // short human readable description, e.g. "swap 3 <-> 7"
};

// Optional recording hook. Production heaps pass nullptr and pay nothing.
class IHeapObserver {
public:
    virtual ~IHeapObserver() = default;
    virtual void onHeapEvent(const HeapEvent& event) = 0;
};

}  // namespace diskscope
