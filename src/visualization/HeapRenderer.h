#pragma once

#include <cstddef>
#include <vector>

#include <imgui.h>

#include "core/FolderNode.h"
#include "structures/HeapEvent.h"

namespace diskscope::viz {

// Level-order tree geometry. `positions[i]` is the centre of the node occupying
// heap slot i and `radii[i]` is its radius: the root is largest and each level
// steps down so nested labels stay readable.
struct HeapLayout {
    std::vector<ImVec2> positions;
    std::vector<float> radii;
    float levelHeight = 0.0f;
    std::size_t depth = 0;
};

// Everything the tree drawing needs to know about the current playback frame.
// The recorder supplies the slots; this struct only decides how they look.
struct HeapDrawState {
    std::size_t highlightA = kNoIndex;
    std::size_t highlightB = kNoIndex;
    std::size_t selected = kNoIndex;
    std::size_t hovered = kNoIndex;
    HeapEventKind kind = HeapEventKind::Done;
    // 0 at the start of a frame, 1 once it has settled. Only used to slide a
    // swapped pair between slots and to grow a freshly inserted node.
    float animT = 1.0f;
};

HeapLayout computeHeapLayout(std::size_t nodeCount, ImVec2 origin, ImVec2 size);

// Slot under `mouse`, or kNoIndex when the pointer is not over a node.
std::size_t hitTestHeapNode(const HeapLayout& layout, ImVec2 mouse);

// Draws edges first, then nodes. Callers set the canvas rect; no border box is
// drawn here because the tree sits directly on the card surface.
void drawHeapTree(ImDrawList* list, const std::vector<const FolderNode*>& array, const HeapLayout& layout,
                  const HeapDrawState& state);

// Fill colour for one slot. Root, compare, swap, insert and extract each get a
// distinct tone from the palette (no extra hues are introduced).
ImU32 slotColour(std::size_t index, const HeapDrawState& state);

// Level-order strip metrics: three stacked lines (index, size, name) per box.
inline constexpr float kArrayBoxWidth = 104.0f;
inline constexpr float kArrayBoxHeight = 62.0f;
inline constexpr float kArrayGap = 6.0f;

float heapArrayContentWidth(std::size_t count);

// Draws the strip with a manual horizontal offset; `clipMinX`/`clipMaxX` cull the
// boxes that scrolled out of the visible area.
void drawHeapArray(ImDrawList* list, const std::vector<const FolderNode*>& array,
                   const HeapDrawState& state, ImVec2 origin, float scrollX, float clipMinX,
                   float clipMaxX);

}  // namespace diskscope::viz
