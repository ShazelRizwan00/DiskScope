#include "core/SizeCalculator.h"

namespace diskscope {

// Post-order aggregation: a directory's inclusive totals are final only after
// every child has returned its own totals, so each node is summed exactly once.
//
//   totals.bytes          = ownFileBytes   + Σ child.totals.bytes
//   totals.allocatedBytes = ownAllocated   + Σ child.totals.allocatedBytes
//   totals.files          = ownFileCount   + Σ child.totals.files
//   totals.folders        = 1              + Σ child.totals.folders   (this node counts)
//
// The node's own aggregate fields are overwritten with the result, which makes
// this function safe to re-run on a partially aggregated tree (for example the
// partial tree left behind by a cancelled scan).
SubtreeTotals accumulateSizes(FolderNode& node) {
    SubtreeTotals totals;
    totals.bytes = node.ownFileBytes;
    totals.allocatedBytes = node.ownAllocatedBytes;
    totals.files = node.ownFileCount;
    totals.folders = 1;

    for (auto& child : node.children) {
        if (!child) {
            continue;
        }
        const SubtreeTotals childTotals = accumulateSizes(*child);
        totals.bytes += childTotals.bytes;
        totals.allocatedBytes += childTotals.allocatedBytes;
        totals.files += childTotals.files;
        totals.folders += childTotals.folders;
    }

    node.totalBytes = totals.bytes;
    node.allocatedBytes = totals.allocatedBytes;
    node.fileCount = totals.files;
    node.folderCount = totals.folders - 1; // descendants only

    return totals;
}

}  // namespace diskscope
