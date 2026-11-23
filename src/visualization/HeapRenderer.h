#pragma once

#include <cstddef>
#include <vector>

#include <imgui.h>

#include "core/FolderNode.h"
#include "structures/HeapEvent.h"

namespace diskscope::viz {

// Geometry of the level-order tree drawing. `positions[i]` is the centre of the
// node occupying heap slot i.
struct HeapLayout {
    std::vector<ImVec2> positions;
    float nodeRadius = 20.0f;
    float levelHeight = 0.0f;
    std::size_t depth = 0;
};

// Classic heap-tree layout: slot i lives at depth floor(log2(i + 1)) and is
// spread evenly inside its level, so parent/child edges never cross.
HeapLayout computeHeapLayout(std::size_t nodeCount, ImVec2 origin, ImVec2 size, float nodeRadius);

// Largest radius that keeps six levels of nodes readable in `size`.
float recommendedNodeRadius(std::size_t nodeCount, ImVec2 size);

// Draws the tree (edges first, then nodes) for a level-order slot array.
// Slots `highlightA`/`highlightB` are tinted according to `kind`, which is what
// makes a comparison or a swap visible in the animation.
void drawHeapTree(ImDrawList* list, const std::vector<const FolderNode*>& array, std::size_t highlightA,
                  std::size_t highlightB, HeapEventKind kind, ImVec2 origin, ImVec2 size);

// Draws the array representation: one box per slot with its index and value.
void drawHeapArray(ImDrawList* list, const std::vector<const FolderNode*>& array, std::size_t highlightA,
                   std::size_t highlightB, HeapEventKind kind, ImVec2 origin, float height);

// Fill colour for one slot. Root, compare, swap, insert and extract each get a
// distinct-but-restrained colour from the theme.
ImU32 slotColour(std::size_t index, std::size_t highlightA, std::size_t highlightB, HeapEventKind kind);

}  // namespace diskscope::viz
