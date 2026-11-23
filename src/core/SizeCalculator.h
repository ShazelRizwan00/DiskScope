#pragma once

#include <cstdint>

#include "core/FolderNode.h"

namespace diskscope {

// Totals gathered while walking one subtree.
struct SubtreeTotals {
    std::uintmax_t bytes = 0;          // inclusive logical bytes
    std::uintmax_t allocatedBytes = 0; // inclusive on-disk bytes
    std::uintmax_t files = 0;          // inclusive regular-file count
    std::uintmax_t folders = 0;        // inclusive folder count (this node counts)
};

// Post-order size aggregation for an already built subtree.
//
// For `node`: totalBytes = ownFileBytes + sum(child.totalBytes),
// fileCount counts every file below the node, folderCount counts descendants.
// The function overwrites the aggregate fields of every node in the subtree and
// returns the totals of the whole subtree.
//
// The scanner calls the same logic while it walks, so this function is the one
// covered by unit tests and by the assignment's dry trace.
SubtreeTotals accumulateSizes(FolderNode& node);

}  // namespace diskscope
