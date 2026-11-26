//Shazel (B)
#pragma once 

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/FileSystemScanner.h"
#include "core/FolderNode.h"
#include "structures/MaxHeap.h"

namespace diskscope {

// Aggregate figures for the Overview dashboard.
struct AnalysisStats {
    std::uintmax_t totalBytes = 0;          // root's inclusive logical size
    std::uintmax_t totalAllocatedBytes = 0; // root's inclusive on-disk size
    std::uintmax_t fileCount = 0;
    std::uintmax_t folderCount = 0;         // descendants of the root
    std::uintmax_t indexedFolders = 0;      // folders currently in the heap
    std::uintmax_t errorCount = 0;
    std::uintmax_t skippedSymlinks = 0;
    std::uintmax_t unreadableFiles = 0;
    const FolderNode* largestFolder = nullptr; // excludes the scan root
    bool rootExcludedFromRankings = true;
    bool cancelled = false;
    bool hasScan = false;
};

// Owns the scanned tree and the ranking heap, and answers the questions the UI
// asks: top-k folders, name search, node lookup, summary statistics.
//
// Thread affinity: main (UI) thread only. The scanner owns its own state on the
// worker thread; a finished ScanResult is handed over by ScanController.
class AnalysisEngine {
public:
    AnalysisEngine() = default;

    // Takes ownership of the tree and rebuilds the id index. Rankings are not
    // rebuilt until rebuildRankings() is called.
    //
    // Ordering requirement (dangling-pointer safety): clear the ranking heap and
    // the id index BEFORE the previous tree is replaced, so no heap slot can
    // ever point into freed nodes between setResult() and rebuildRankings().
    // generation() changes on every setResult()/clear() so views can drop cached
    // node pointers, path selections and demo frames.
    void setResult(ScanResult result);
    void clear();

    // Rebuilds the ranking heap from a bottom-up build over the root's
    // descendants (the scan root itself is excluded unless includeRoot is true,
    // because it is by definition the largest folder in its own scan).
    void rebuildRankings(bool includeRoot = false);

    // Descending top-k folders, straight from the ranking heap.
    std::vector<const FolderNode*> topK(std::size_t k) const;

    // Case-insensitive substring match on folder names, largest first.
    std::vector<const FolderNode*> search(const std::string& term, std::size_t limit = 20) const;

    const FolderNode* find(NodeId id) const;
    const ScanResult& result() const { return result_; }
    const MaxHeap& heap() const { return heap_; }
    AnalysisStats stats() const;
    bool hasResult() const { return result_.root != nullptr; }
    std::uint64_t generation() const { return generation_; }

    // Parent chain from the scan root to `node`, root first (empty if unknown).
    std::vector<const FolderNode*> pathTo(const FolderNode* node) const;

private:
    void indexSubtree(FolderNode& node);

    // Derives the statistics from result_, heap_.size() and includeRoot_.
    AnalysisStats computeStats() const;

    ScanResult result_;
    std::unordered_map<NodeId, FolderNode*> index_;
    // Ids are assigned in discovery order, so this doubles as discovery order.
    std::vector<FolderNode*> discoveryOrder_;
    MaxHeap heap_;
    bool includeRoot_ = false;
    std::uint64_t generation_ = 0;

    // stats() is derived state and the UI asks for it several times per frame, so
    // it is computed once and invalidated by every mutator that feeds it.
    mutable AnalysisStats statsCache_;
    mutable bool statsCacheValid_ = false;
};

}  // namespace diskscope
